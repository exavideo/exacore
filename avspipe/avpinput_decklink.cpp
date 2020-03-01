/*
 * Copyright 2011 Andrew H. Armenia.
 *
 * This file is part of openreplay.
 *
 * openreplay is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * openreplay is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with openreplay.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "decklink.h"
#include "raw_frame.h"
#include "rsvg_frame.h"
#include "packed_audio_packet.h"
#include "pipe.h"
#include "thread.h"
#include "xmalloc.h"
#include "mjpeg_codec.h"
#include "svg_subprocess_character_generator.h"
#include "png_subprocess_character_generator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <getopt.h>

#include <sstream>

char *parse_command(const char *cmd, int vpfd, int apfd) {
    size_t len = strlen(cmd);
    size_t out_len = 2*len;
    char *out = (char *)xmalloc(out_len + 1,
            "avpinput_decklink", "command buffer");
    char *outp = out;
    char number[80];
    int number_size;
    char ch;

    memset(out, 0, out_len + 1);

    while (*cmd) {
        /*
         * Consume one character of input.
         * This leaves *cmd at the character after the one
         * currently being acted upon.
         */
        ch = *cmd;
        cmd++;

        if (ch == '%') {
            /*
             * A % sign introduces an escape sequence.
             * Look at the next character to decide what it means.
             */
            if (*cmd == '%') {
                /* %%: literal % sign */
                if (out_len > 0) {
                    *outp = '%';
                    outp++;
                    out_len--;
                }
                cmd++;
            } else if (*cmd == '\0') {
                /*
                 * a lonely '%' sign at the end of
                 * the string cannot be parsed
                 */
                if (out_len > 0) {
                    *outp = '%';
                    outp++;
                    out_len--;
                }
            } else if (*cmd == 'a') {
                /* %a: substitute audio fd */
                number_size = snprintf(number, sizeof(number), "%d", apfd);
                if (number_size >= 0 && out_len >= (size_t) number_size) {
                    memcpy(outp, number, number_size);
                    outp += number_size;
                    out_len -= number_size;
                }
                cmd++;
            } else if (*cmd == 'v') {
                /* %v: substitute video fd */
                number_size = snprintf(number, sizeof(number), "%d", vpfd);
                if (number_size >= 0 && out_len >= (size_t) number_size) {
                    memcpy(outp, number, number_size);
                    outp += number_size;
                    out_len -= number_size;
                }
                cmd++;
            } else {
                /* it doesn't match... so just use it as a literal */
                if (out_len > 0) {
                    *outp = '%';
                    outp++;
                    out_len--;
                }
            }
        } else {
            /* literal character... we just pass it through */
            if (out_len > 0) {
                *outp = ch;
                outp++;
                out_len--;
            }
        }
    }

    return out;
}

pid_t start_subprocess(const char *cmd, int &vpfd, int &apfd) {
    pid_t child;

    int vpipe[2], apipe[2];

    if (pipe2(vpipe, O_CLOEXEC) != 0) {
        perror("pipe(vpipe)");
        return -1;
    }

    if (pipe2(apipe, O_CLOEXEC) != 0) {
        /*
         * FIXME a pair of FDs leak here if there is an error.
         * Does not matter now.
         */
        perror("pipe(apipe)");
        return -1;
    }

    char *cmd_to_exec = parse_command(cmd, vpipe[0], apipe[0]);

    child = fork( );

    if (child == -1) {
        /* fork( ) failed */
        perror("fork");
        return -1;
    } else if (child == 0) {
        // let these two FDs be inherited
        fcntl(vpipe[0], F_SETFD, 0);
        fcntl(apipe[0], F_SETFD, 0);
        execl("/bin/sh", "/bin/sh", "-c", cmd_to_exec, NULL);
        perror("execl");
        exit(1);
    } else {
        /* parent */
        close(vpipe[0]);
        close(apipe[0]);
        vpfd = vpipe[1];
        apfd = apipe[1];
        free(cmd_to_exec);

        return child;
    }
}

template <class Thing>
class Filter {
    public:
        virtual void filter(Thing *thing) = 0;
};

class BugFilter : public Filter<RawFrame> {
    public:
        BugFilter(const char *svgfile) {
            int fd;
            char *data;
            size_t size;
            struct stat statbuf;

            fd = open(svgfile, O_RDONLY);
            if (fd == -1) {
                throw std::runtime_error("could not open svg file");
            }

            if (fstat(fd, &statbuf) == -1) {
                throw std::runtime_error("could not stat svg file");
            }

            size = statbuf.st_size;
            data = new char[size];

            if (read_all(fd, data, size) != 1) {
                throw std::runtime_error("could not read svg file");
            }

            svgframe = RsvgFrame::render_svg(data, size);

            delete data;
        }

        ~BugFilter() {
            delete svgframe;
        }

        virtual void filter(RawFrame *thing) {
            thing->draw->alpha_key(0, 0, svgframe, 255);
        }
    protected:
        RawFrame *svgframe;
};

class CgFilter : public Filter<RawFrame> {
    public:
        CgFilter(CharacterGenerator *cg) {
            this->cg = cg;
        }

        virtual void filter(RawFrame *thing) {
            RawFrame *key = cg->output_pipe( ).get( ).frame;

            if (key != NULL) {
                thing->draw->alpha_key(cg->x( ), cg->y( ),
                    key, key->global_alpha( ));

                delete key;
            }
        }

    protected:
        CharacterGenerator *cg;
};

class PreviewFile {
    public:
        PreviewFile(const char *extension) {
            if (asprintf(&output_filename, "/tmp/avpinput_%d.%s",
                    getpid( ), extension) == -1) {
                fd_ = -1;
                return;
            }

            if (asprintf(&tmp_filename, "%s.tmp", output_filename) == -1) {
                free(output_filename);
                fd_ = -1;
                return;
            }

            fd_ = open(tmp_filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        }

        ~PreviewFile( ) {
            if (fd_ != -1) {
                close(fd_);
                rename(tmp_filename, output_filename);
                free(output_filename);
                free(tmp_filename);
            }
        }

        bool ok( ) {
            return fd_ != -1;
        }

        int fd( ) {
            return fd_;
        }

    protected:
        int fd_;
        char *output_filename;
        char *tmp_filename;
};

class PreviewFilter : public Filter<RawFrame> {
    public:
        PreviewFilter()
            : enc(480, 272, 80, 16*1024*1024),
            counter(0) {

        }

        virtual void filter(RawFrame *thing) {
            counter++;

            if (counter == 30) {
                counter = 0;
                RawFrame *small =
                    thing->convert->CbYCrY8422_scaled(480, 270);
                enc.encode(small);
                delete small;

                PreviewFile output("jpg");
                if (output.ok( )) {
                    /* deliberately ignoring errors here. */
                    write_all(output.fd( ), enc.get_data( ),
                        enc.get_data_size( ));
                }
            }
        }
    protected:
        Mjpeg422Encoder enc;
        int counter;
};

class AudioLevelsFilter : public Filter<IOAudioPacket> {
    public:
        AudioLevelsFilter( ) {
            peaks = NULL;
            counter = 0;
        }

        void filter(IOAudioPacket *pkt) {
            if (peaks == NULL) {
                peaks = new int32_t[pkt->channels( )];
                n_channels = pkt->channels( );
                for (size_t i = 0; i < n_channels; i++) {
                    peaks[i] = 0;
                }
            }

            if (pkt->channels( ) != n_channels) {
                return;
            }

            /* peak decay */
            for (size_t i = 0; i < pkt->channels( ); i++) {
                peaks[i] = peaks[i] * 9 / 10;
            }

            /* peak search in this packet */
            int16_t *sample_ptr = pkt->data( );
            for (size_t i = 0; i < pkt->size_samples( ); i++) {
                for (size_t j = 0; j < pkt->channels( ); j++) {
                    if (sample_ptr[j] > peaks[j]) {
                        peaks[j] = sample_ptr[j];
                    }
                }
                sample_ptr += n_channels;
            }

            counter++;
            if (counter == 30) {
                counter = 0;
                PreviewFile out("peak");
                if (out.ok( )) {
                    std::ostringstream outstr;
                    for (size_t i = 0; i < n_channels; i++) {
                        float dbfs = 20 * log10f(peaks[i] / 32768.0);
                        outstr << dbfs << std::endl;
                    }

                    write_all(out.fd( ), outstr.str( ).c_str( ),
                        outstr.str( ).size( ));
                }
            }
        }

    protected:
        int32_t *peaks;
        size_t n_channels;
        int counter;
};

class DeckLinkOutputFilter : public Filter<RawFrame> {
    public:
        DeckLinkOutputFilter(int card) {
            oadp = create_decklink_output_adapter(card, 0, RawFrame::CbYCrY8422);
        }

        void filter(RawFrame *f) {
            oadp->input_pipe().put(f->copy());
        }

    private:
        OutputAdapter *oadp;
};

template <class T>
class FilterChain : public Filter<T> {
    public:
        FilterChain() {

        }

        ~FilterChain() {

        }

        virtual void filter(T *thing) {
            for (Filter<T> *&filt : filters) {
                filt->filter(thing);
            }
        }

        void add_filter(Filter<T> *filt) {
            filters.push_back(filt);
        }

    protected:
        std::vector<Filter<T> *> filters;
};

template <class SendableThing>
class SenderThread : public Thread {
    public:
        SenderThread(Pipe<SendableThing *> *fpipe, int out_fd,
            Filter<SendableThing> *filt = NULL
        ) {
            assert(fpipe != NULL);
            assert(out_fd >= 0);

            _fpipe = fpipe;
            _out_fd = out_fd;
            _filter = filt;
        }

        void start( ) {
            start_thread( );
        }

    protected:
        void run_thread(void) {
            SendableThing *thing;

            for (;;) {
                thing = _fpipe->get( );

                if (_filter) {
                    _filter->filter(thing);
                }

                if (thing->write_to_fd(_out_fd) <= 0) {
                    fprintf(stderr, "write failed or pipe broken\n");
                    break;
                }

                delete thing;
            }
        }

        Pipe<SendableThing *> *_fpipe;
        int _out_fd;
        Filter<SendableThing> *_filter;
        int _preroll_frames;
};

class CompressorThread : public SenderThread<RawFrame> {
    public:
        CompressorThread(Pipe<RawFrame *> *fpipe, int out_fd,
                Filter<RawFrame> *filt = NULL, int qual = 80)
            : SenderThread(fpipe, out_fd, filt),
            enc(1920, 1080, qual, 16*1024*1024) { }

    protected:
        void run_thread(void) {
            RawFrame *f;

            for (;;) {
                f = _fpipe->get( );

                if (_filter) {
                    _filter->filter(f);
                }

                enc.encode(f);
                delete f;
                write_all(_out_fd, enc.get_data( ), enc.get_data_size( ));
            }
        }

        Mjpeg422Encoder enc;
};

/* Filter to mix together all audio channels to mono. */
class MixdownFilter : public Filter<IOAudioPacket> {
    public:
        void filter(IOAudioPacket *pkt) {
            int32_t sum;
            int16_t *sample_ptr = pkt->data( );

            for (size_t i = 0; i < pkt->size_samples( ); i++) {
                sum = 0;
                for (size_t j = 0; j < pkt->channels( ); j++) {
                    sum += sample_ptr[j];
                }

                sum /= pkt->channels( );

                for (size_t j = 0; j < pkt->channels( ); j++) {
                    sample_ptr[j] = sum;
                }

                sample_ptr += pkt->channels( );
            }
        }
};

void usage(const char *argv0) {
    fprintf(stderr, "usage: %s [-c n] [-f] 'command'\n", argv0);
    fprintf(stderr, "-c n: use card 'n'\n");
    fprintf(stderr, "-f: enable ffmpeg streaming hack\n");
    fprintf(stderr, "-j: output M-JPEG instead of raw video\n");
    fprintf(stderr, "-q [0-100]: M-JPEG quality scale\n");
    fprintf(stderr, "-o n: output to DeckLink device 'n'\n");
    fprintf(stderr, "-P: write preview jpegs to /tmp\n");
    fprintf(stderr, "--channels n: record n audio channels (default 2)\n");
    fprintf(stderr, "in 'command':\n");
    fprintf(stderr, "%%a = audio pipe fd\n");
    fprintf(stderr, "%%v = video pipe fd\n");
}

int main(int argc, char * const *argv) {
    InputAdapter *iadp;

    int vpfd, apfd, opt;
    pid_t child;
    CharacterGenerator *cg;
    coord_t x = 0, y = 0;

    Pipe<IOAudioPacket *> *apipe;
    FilterChain<RawFrame> filter_chain;
    FilterChain<IOAudioPacket> audio_filter_chain;

    static struct option options[] = {
        { "card", 1, 0, 'c' },
        { "bug", 1, 0, 'b' },
        { "channels", 1, 0, 'C' },
        { "svg-cg", 1, 0, 's' },
        { "png-cg", 1, 0, 'P' },
        { "cg-x", 1, 0, 'x' },
        { "cg-y", 1, 0, 'y' },
        { 0, 0, 0, 0 }
    };

    int card = 0;
    int audio_channels = 2;
    int jpeg = 0;
    int quality = 80;
    int output_card;

    /* argument processing */
    while ((opt = getopt_long(argc, argv, "o:Rmjfc:b:q:", options, NULL)) != -1) {
        switch (opt) {
            case 'm':
                audio_filter_chain.add_filter(new MixdownFilter);
                break;

            case 'c':
                card = atoi(optarg);
                break;

            case 'b':
                filter_chain.add_filter(new BugFilter(optarg));
                break;

            case 'R':
                filter_chain.add_filter(new PreviewFilter);
                audio_filter_chain.add_filter(new AudioLevelsFilter);
                break;

            case 'C':
                audio_channels = atoi(optarg);
                break;

            case 'j':
                jpeg = 1;
                break;

            case 'q':
                quality = atoi(optarg);
                if (quality < 0 || quality > 100) {
                    usage(argv[0]);
                    exit(1);
                }
                break;

            case 's':
                cg = new SvgSubprocessCharacterGenerator(optarg);
                cg->set_x(x);
                cg->set_y(y);
                filter_chain.add_filter(new CgFilter(cg));
                break;

            case 'P':
                cg = new PngSubprocessCharacterGenerator(optarg);
                cg->set_x(x);
                cg->set_y(y);
                filter_chain.add_filter(new CgFilter(cg));
                break;

            case 'x':
                x = atoi(optarg);
                break;

            case 'y':
                y = atoi(optarg);
                break;

            case 'o':
                // note annoyance: this preview output doesn't support audio
                // (this will eventually, maybe, be solved by avipc)
                output_card = atoi(optarg);
                filter_chain.add_filter(new DeckLinkOutputFilter(output_card));
                break;

            default:
                usage(argv[0]);
                exit(1);
        }
    }

    if (argc - optind != 1) {
        usage(argv[0]);
        exit(1);
    }

    child = start_subprocess(argv[optind], vpfd, apfd);

    if (child == -1) {
        return 1;
    }


    iadp = create_decklink_input_adapter_with_audio(card, 0, 0,
            RawFrame::CbYCrY8422, audio_channels);
    apipe = iadp->audio_output_pipe( );

    SenderThread<RawFrame> *vsthread;
    SenderThread<IOAudioPacket> *asthread;
    /* start video and audio sender threads */
    if (jpeg) {
        vsthread = new CompressorThread(
            &(iadp->output_pipe( )), vpfd,
            &filter_chain, quality
        );
    } else {
        vsthread = new SenderThread<RawFrame>(
            &(iadp->output_pipe( )), vpfd,
            &filter_chain
        );
    }

    asthread = new SenderThread<IOAudioPacket>(
        apipe, apfd,
        &audio_filter_chain
    );

    asthread->start( );
    vsthread->start( );

    iadp->start( );

    /* wait on child process */
    while (waitpid(child, NULL, 0) == EINTR) { /* busy wait */ }

    /* done! */
}
