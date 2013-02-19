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
#include "pipe.h"
#include "thread.h"
#include "mjpeg_codec.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>

sig_atomic_t flag = 0;

uint64_t time_usec( ) {
    struct timespec ts;
    uint64_t ret;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        perror("clock_gettime");
        return 0;
    } else {
        ret = ts.tv_sec;
        ret *= 1000000;
        ts.tv_nsec /= 1000;
        ret += ts.tv_nsec;
        return ret;
    }
}

void print_fps(void *id, uint64_t start, uint64_t end, int n_frames) {
    uint64_t total_time = end - start;
    float seconds = total_time / 1000000.0f;
    float fps = n_frames / seconds;

    fprintf(stderr, "[%p] %d frames, %f sec, averaged %f fps\n", 
        id, n_frames, seconds, fps);
}

class MjpegCaptureThread : public Thread {
public:
    MjpegCaptureThread(InputAdapter *iadp_) {
        iadp = iadp_;
        start_thread( );
    }

    ~MjpegCaptureThread( ) {
        join_thread( );
    }
protected:
    InputAdapter *iadp;

    void run_thread( ) {
        Mjpeg422Encoder enc(1920, 1080);
        int fd, ret;
        char filename[1024];
        uint64_t start_usec, end_usec;

        RawFrame *fr;
        AudioPacket *ap;

        int frameno = 0;
        fprintf(stderr, "[%p] thread starting\n", this);
        sprintf(filename, "/mnt/cam1/capture_%p.mjpg", this);
        //sprintf(filename, "/dev/null");
        fprintf(stderr, "[%p] writing to %s\n", this, filename);

        fd = open(filename, O_CREAT | O_WRONLY, 0644);
        //fd = open(filename, O_WRONLY, 0644);
        if (fd < 0) {
            perror("open");
            return;
        }
        
        /* get time of day */
        start_usec = time_usec( );

        while (flag == 0) {
            fr = iadp->output_pipe( ).get( );
            if (fr == NULL) {
                fprintf(stderr, "%p: frame was null\n", this);
            } else {
                /* encode frame as M-JPEG */
                enc.encode(fr);
                delete fr;

                /* write to fd */

                ret = write_all(fd, enc.get_data( ), enc.get_data_size( ));

                if (ret < 0) {
                    perror("write_all");
                    return;
                }
            }

            /* discard audio data */
            ap = iadp->audio_output_pipe( )->get( );
            delete ap;

            frameno++;
        }

        /* get time of day again and print message */
        end_usec = time_usec( );
        print_fps(this, start_usec, end_usec, frameno);
    }
};

class NullCaptureThread : public Thread {
public:
    NullCaptureThread(InputAdapter *iadp_) {
        iadp = iadp_;
        start_thread( );
    }
protected:
    InputAdapter *iadp;
    void run_thread( ) {
        RawFrame *fr;
        AudioPacket *ap;
        for (;;) {
            fr = iadp->output_pipe( ).get( );            
            //fprintf(stderr, "deleting %p\n", fr);
            delete fr;

            ap = iadp->audio_output_pipe( )->get( );
            delete ap;
        }
    }
};

class CopyCaptureThread : public Thread {
    public:
        CopyCaptureThread(InputAdapter *iadp_, OutputAdapter *oadp_) {
            iadp = iadp_;
            oadp = oadp_;
            start_thread( );
        }

        ~CopyCaptureThread( ) {
            join_thread( );
        }
    protected:
        InputAdapter *iadp;
        OutputAdapter *oadp;

        void run_thread( ) {
            RawFrame *fr;
            AudioPacket *ap;

            while (flag == 0) {
                fr = iadp->output_pipe( ).get( );
                oadp->input_pipe( ).put(fr);

                ap = iadp->audio_output_pipe( )->get( );
                oadp->audio_input_pipe( )->put(ap);
            }
        }
};

void sigint(int sig) {
    (void) sig;
    flag = 1;
}

int main( ) {
    const int n = 7;

    InputAdapter *iadps[n];
    OutputAdapter *oadp;
    Thread *threads[n];

    for (int i = 0; i < n; i++) {
        iadps[i] = create_decklink_input_adapter_with_audio(i, 0, 0, RawFrame::CbYCrY8422);
        if (i == 0) {
            oadp = create_decklink_output_adapter_with_audio(7, 0, RawFrame::CbYCrY8422);
            threads[i] = new CopyCaptureThread(iadps[i], oadp);
        } else /* if (i < 4) */ {
            threads[i] = new MjpegCaptureThread(iadps[i]);
        } /*else {
            threads[i] = new NullCaptureThread(iadps[i]);
        }*/
        iadps[i]->start( );
    }

    /* wait here until we catch a SIGINT */
    signal(SIGINT, sigint);
    while (flag == 0) {
        sleep(1);
    }

    fprintf(stderr, "SIGINT caught, cleaning up...\n");
    flag = 1;
    for (int i = 0; i < n; i++) {
        //threads[i]->join( );
        delete threads[i];
    }
}

