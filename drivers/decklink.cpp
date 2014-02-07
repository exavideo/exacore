/*
 * Copyright 2011, 2013 Andrew H. Armenia.
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

#include "adapter.h"

#include "DeckLinkAPI.h"
#include "types.h"
#include "audio_fifo.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <time.h> // debug

#define IN_PIPE_SIZE 256
#define OUT_PIPE_SIZE 4

struct decklink_norm {
    const char *name;
    BMDTimeScale time_base;
    BMDTimeValue frame_duration;
    BMDDisplayMode mode;
    coord_t w, h;
};

struct decklink_connection {
    const char *name;
    BMDVideoConnection connection;
};

struct decklink_pixel_format {
    RawFrame::PixelFormat pf;
    BMDPixelFormat bpf;
    bool end;
};

static struct decklink_norm norms[] = {
    /* This list is not exhaustive. */
    { "1080i 59.94", 30000, 1001, bmdModeHD1080i5994, 1920, 1080 },
    { "NTSC", 30000, 1001, bmdModeNTSC, 720, 486 },
};

static struct decklink_connection connections[] = {
    /* This one is, though. */
    { "SD/HD-SDI (copper)", bmdVideoConnectionSDI },
    { "SD/HD-SDI (fiber)", bmdVideoConnectionOpticalSDI }, 
    { "HDMI", bmdVideoConnectionHDMI },
    { "Analog Component", bmdVideoConnectionComponent },
    { "Analog S-Video", bmdVideoConnectionSVideo },
    { "Analog Composite", bmdVideoConnectionComposite }
};

static struct decklink_pixel_format pfs[] = {
    { RawFrame::CbYCrY8422, bmdFormat8BitYUV, false },
    { RawFrame::BGRAn8, bmdFormat8BitARGB, true }
};


static IDeckLink *find_card(int card_index) {
    HRESULT ret;
    IDeckLinkIterator *dli = CreateDeckLinkIteratorInstance( );
    IDeckLink *dl;

    if (!dli) {
        throw std::runtime_error(
            "CreateDeckLinkIteratorInstance( ) failed"
        );
    }

    while (card_index >= 0) {
        ret = dli->Next(&dl);
        if (ret != S_OK) {
            throw std::runtime_error("DeckLink card not found");
        }
        card_index--;
    }

    return dl;
}

static BMDPixelFormat convert_pf(RawFrame::PixelFormat in_pf) {
    struct decklink_pixel_format *pfm;

    for (pfm = pfs; ; pfm++) {
        if (pfm->pf == in_pf) {
            return pfm->bpf;
        }

        if (pfm->end) {
            throw std::runtime_error(
                "DeckLink: pixel format unsupported"
            );
        }
    }
}

/* lookup the field dominance corresponding to this BMDDisplayMode */
static RawFrame::FieldDominance find_dominance(BMDDisplayMode mode, 
        IDeckLinkDisplayModeIterator *iterator) {
    IDeckLinkDisplayMode *imode;

    if (iterator->Next(&imode) != S_OK) {
        throw std::runtime_error("DeckLink: failed to iterate display modes");
    }

    while (imode) {
        BMDFieldDominance fd = imode->GetFieldDominance( );
        BMDDisplayMode thismode = imode->GetDisplayMode( );
        imode->Release( );

        if (thismode == mode) {
            switch (fd) {
                case bmdLowerFieldFirst:
                    return RawFrame::BOTTOM_FIELD_FIRST;
                case bmdUpperFieldFirst:
                    return RawFrame::TOP_FIELD_FIRST;
                case bmdProgressiveFrame:
                case bmdProgressiveSegmentedFrame:
                    return RawFrame::PROGRESSIVE;
                default:
                    return RawFrame::UNKNOWN;
            }
        }

        if (iterator->Next(&imode) != S_OK) {
            throw std::runtime_error("failed to iterate display modes");
        }
    }

    /* no modes matched so we don't know dominance */
    return RawFrame::UNKNOWN; 
}

RawFrame *create_raw_frame_from_decklink(IDeckLinkVideoFrame *frame,
        RawFrame::PixelFormat pf) {
    void *dp;
    RawFrame *ret = new RawFrame(
        frame->GetWidth( ),
        frame->GetHeight( ),
        pf, frame->GetRowBytes( )
    );

    if (frame->GetBytes(&dp) != S_OK) {
        throw std::runtime_error("Cannot get pointer to raw data");
    }

    memcpy(ret->data( ), dp, ret->size( ));

    return ret;
}
/* Adapter from IDeckLinkVideoFrame to RawFrame, enables zero-copy input */
class DecklinkInputRawFrame : public RawFrame {
    public:
        DecklinkInputRawFrame(IDeckLinkVideoFrame *frame, 
                RawFrame::PixelFormat pf) : RawFrame(pf) {

            void *dp;

            assert(frame != NULL);

            _frame = frame;

            _frame->AddRef( ); 

            if (_frame->GetBytes(&dp) != S_OK) {
                throw std::runtime_error("Cannot get pointer to raw data");
            }

            _data = (uint8_t *) dp;
            _w = _frame->GetWidth( );
            _h = _frame->GetHeight( );
            _pitch = _frame->GetRowBytes( );            
        }

        virtual ~DecklinkInputRawFrame( ) {
            _frame->Release( );
            _frame = NULL;
            _data = NULL;
        }
    protected:
        IDeckLinkVideoFrame *_frame;

        virtual void alloc( ) {
            throw std::runtime_error(
                "Cannot allocate a DecklinkInputRawFrame"
            );
        }
};

class DeckLinkOutputAdapter : public OutputAdapter, 
        public IDeckLinkVideoOutputCallback,
        public IDeckLinkAudioOutputCallback {

    public:
        DeckLinkOutputAdapter(unsigned int card_index = 0, 
                unsigned int norm_ = 0, 
                RawFrame::PixelFormat pf_ = RawFrame::CbYCrY8422,
                bool enable_audio = false, unsigned int n_channels = 2) 
                : deckLink(NULL), 
                deckLinkOutput(NULL), frame_counter(0),
                last_frame(NULL), in_pipe(OUT_PIPE_SIZE), audio_in_pipe(NULL) {

            frames_written = 0;
            audio_adjust = 0;
            norm = norm_;
            assert(norm < sizeof(norms) / sizeof(struct decklink_norm));
            time_base = norms[norm].time_base;
            frame_duration = norms[norm].frame_duration;

            pf = pf_;
            bpf = convert_pf(pf_); 

            deckLink = find_card(card_index);
            configure_card( );
            open_card( );
            preroll_video_frames(5);

            if (enable_audio) {
                this->n_channels = n_channels;
                setup_audio( );
            } else {
                this->n_channels = 0;
            }

            start_video( );

            //thread_priority_hack( );

            fprintf(stderr, "DeckLink: initialized using norm %s\n", 
                    norms[norm].name);
        }

        void thread_priority_hack( ) {            
            /* abuse the crap out of the DeckLink API pointers... */
            struct sched_param param;
            param.sched_priority = 20;
            uint8_t *dlmem = (uint8_t *)deckLink;

            /* these offsets were found via GDB and could change at any time! */
            pthread_t thread_a = *(pthread_t *)(dlmem + 0x110);
            pthread_t thread_b = *(pthread_t *)(dlmem + 0x230);


            /* change the threads to round-robin scheduler */
            if (pthread_setschedparam(thread_a, SCHED_FIFO, &param) != 0) {
                perror("pthread_setschedparam");
                fprintf(stderr, "cannot set FIFO scheduler for DL-A thread");
            }
            
            if (pthread_setschedparam(thread_b, SCHED_FIFO, &param) != 0) {
                perror("pthread_setschedparam");
                fprintf(stderr, "cannot set RR scheduler for DL-B thread");
            }

        }

        /* DeckLink callbacks */
        virtual HRESULT ScheduledFrameCompleted(IDeckLinkVideoFrame *frame,
                BMDOutputFrameCompletionResult result) {

            switch (result) {
                case bmdOutputFrameDisplayedLate:
                    fprintf(stderr, "WARNING: Decklink displayed frame late (running too slow!)\r\n");
                    break;
                case bmdOutputFrameDropped:
                    fprintf(stderr, "WARNING: Decklink dropped frame\r\n");
                    break;
                case bmdOutputFrameFlushed:
                    fprintf(stderr, "WARNING: Decklink flushed frame\r\n");
                    break;
                default:
                    break;
            }

            schedule_frame((IDeckLinkMutableVideoFrame *) frame);

            return S_OK;
        }

        /* stubs */
        virtual HRESULT QueryInterface(REFIID iid, void **lpv) {
            UNUSED(iid);
            UNUSED(lpv);
            return E_FAIL;
        }

        virtual ULONG AddRef( ) {
            return 1;
        }

        virtual ULONG Release( ) {
            return 1;
        }

        virtual HRESULT ScheduledPlaybackHasStopped(void) {
            fprintf(stderr, "DeckLink: warning: playback stopped\n");
            return S_OK;
        }

        virtual HRESULT RenderAudioSamples(bool preroll) {
            IOAudioPacket *audio;
            uint32_t n_consumed;

            if (!preroll) {
                /* read from audio input pipe */
                while (audio_in_pipe->data_ready( )) {
                    /* maybe include a high-water mark to stop loop? */

                    if (audio_adjust > 0) {
                        /* 
                         * insert dummy audio as needed to compensate for 
                         * dropped video frames
                         */
                        audio = new IOAudioPacket(1601, n_channels);
                        audio->zero( );
                        audio_adjust--;
                    } else {
                        audio = audio_in_pipe->get( );
                    }
                    audio_fifo->add_packet(audio);
                    delete audio;
                }
            }

            if (audio_fifo->fill_samples( ) > 0) {
                deckLinkOutput->ScheduleAudioSamples(audio_fifo->data( ), 
                        audio_fifo->fill_samples( ), 0, 0, &n_consumed);
                audio_fifo->pop_samples(n_consumed);
            } else if (preroll) {
                audio_preroll_done = 1;
            }

            return S_OK;
        }

        Pipe<RawFrame *> &input_pipe( ) { return in_pipe; }
        Pipe<IOAudioPacket *> *audio_input_pipe( ) { return audio_in_pipe; }
        AudioFIFO<int16_t> *audio_fifo;

        RawFrame::FieldDominance output_dominance( ) { return dominance; }
    
    protected:        
        IDeckLink *deckLink;
        IDeckLinkOutput *deckLinkOutput;
        
        BMDTimeScale time_base;
        BMDTimeValue frame_duration;
        unsigned int frame_counter;

        unsigned int norm;
        unsigned int audio_adjust;

        RawFrame *last_frame;

        Pipe<RawFrame *> in_pipe;

        RawFrame::PixelFormat pf;
        RawFrame::FieldDominance dominance;
        BMDPixelFormat bpf;

        volatile int audio_preroll_done;
        unsigned int n_channels;
        Pipe<IOAudioPacket *> *audio_in_pipe;

        IOAudioPacket *current_audio_pkt;
        uint32_t samples_written_from_current_audio_pkt;
        uint32_t frames_written;

        void open_card( ) {
            IDeckLinkDisplayModeIterator *it;

            /* get the DeckLinkOutput interface */
            if (deckLink->QueryInterface(IID_IDeckLinkOutput, 
                    (void **)&deckLinkOutput) != S_OK) {
                
                throw std::runtime_error(
                    "Failed to get DeckLink output interface handle"
                );
            }
            
            if (deckLinkOutput->SetScheduledFrameCompletionCallback(this) 
                   != S_OK) {

                throw std::runtime_error(
                    "Failed to set DeckLink frame completion callback"
                );
            }

            /* attempt to determine field dominance */
            if (deckLinkOutput->GetDisplayModeIterator(&it) != S_OK) {
                throw std::runtime_error(
                    "DeckLink output: failed to get display mode iterator"
                );
            }
            
            dominance = find_dominance(norms[norm].mode, it);

            it->Release( );

            /* and we're off to the races */
            if (deckLinkOutput->EnableVideoOutput(norms[norm].mode,
                    bmdVideoOutputFlagDefault) != S_OK) {
                
                throw std::runtime_error(
                    "Failed to enable DeckLink video output"
                );
            }
        }

        void setup_audio( ) {
            IOAudioPacket preroll_audio(8008, n_channels);
            preroll_audio.zero( );

            audio_in_pipe = new Pipe<IOAudioPacket *>(OUT_PIPE_SIZE);
            audio_fifo = new AudioFIFO<int16_t>(n_channels);
            audio_fifo->add_packet(&preroll_audio);

            assert(deckLinkOutput != NULL);

            if (deckLinkOutput->SetAudioCallback(this) != S_OK) {
                throw std::runtime_error(
                    "Failed to set DeckLink audio callback"
                );
            }

            if (deckLinkOutput->EnableAudioOutput( 
                    bmdAudioSampleRate48kHz, bmdAudioSampleType16bitInteger, 
                    n_channels, bmdAudioOutputStreamContinuous) != S_OK) {
                throw std::runtime_error(
                    "Failed to enable DeckLink audio output"
                );
            }

            audio_preroll_done = 0;

            if (deckLinkOutput->BeginAudioPreroll( ) != S_OK) {
                throw std::runtime_error(
                    "Failed to begin DeckLink audio preroll"
                );
            }

            while (audio_preroll_done == 0) { 
                /* FIXME: busy wait */
            }

            if (deckLinkOutput->EndAudioPreroll( ) != S_OK) {
                throw std::runtime_error(
                    "Failed to end DeckLink audio preroll"
                );
            }
        }

        void configure_card( ) {
            IDeckLinkConfiguration *config;

            assert(deckLink != NULL);

            if (deckLink->QueryInterface(IID_IDeckLinkConfiguration, 
                    (void**) &config) != S_OK) {
                
                throw std::runtime_error(
                    "DeckLink output: get IDeckLinkConfiguration failed"
                );
            }

            if (config->SetInt(bmdDeckLinkConfigLowLatencyVideoOutput, true)
                    != S_OK) {

                fprintf(stderr, "DeckLink output: warning: "
                        "cannot enable low-latency mode\n"
                );
            }

            if (config->SetInt(bmdDeckLinkConfigVideoOutputConversionMode,
                    bmdVideoOutputHardwareLetterboxDownconversion) != S_OK) {
                fprintf(stderr, "DeckLink output: warning: "
                        "cannot enable hardware letterboxing\n");
            }

            if (config->SetInt(bmdDeckLinkConfigBypass, -1) != S_OK) {
                fprintf(stderr, "DeckLink output: warning: "
                        "cannot deactivate card bypass relay\n"
                );
            }
                    
            /* throw outputs at wall, see what sticks :) */
            #if 0
            config->SetInt(bmdDeckLinkConfigVideoOutputConnection,
                bmdVideoConnectionSDI);
            config->SetInt(bmdDeckLinkConfigVideoOutputConnection,
                bmdVideoConnectionHDMI);
            config->SetInt(bmdDeckLinkConfigVideoOutputConnection,
                bmdVideoConnectionComponent);
            config->SetInt(bmdDeckLinkConfigVideoOutputConnection,
                bmdVideoConnectionComposite);
            config->SetInt(bmdDeckLinkConfigVideoOutputConnection,
                bmdVideoConnectionSVideo);
            #endif

            config->Release( );
        }


        void preroll_video_frames(unsigned int n_frames) {
            IDeckLinkMutableVideoFrame *frame;
            IDeckLinkVideoFrameAncillary *anc;
            for (unsigned int i = 0; i < n_frames; i++) {
                if (deckLinkOutput->CreateVideoFrame(norms[norm].w, 
                        norms[norm].h, 2*norms[norm].w, bpf,
                        bmdFrameFlagDefault, &frame) != S_OK) {

                    throw std::runtime_error("Failed to create frame"); 
                }

                if (deckLinkOutput->CreateAncillaryData(bpf, &anc) != S_OK) {
                    throw std::runtime_error("failed to set frame ancillary data");
                }

                if (frame->SetAncillaryData(anc) != S_OK) {
                    throw std::runtime_error("failed to set frame ancillary data");
                }

                schedule_frame(frame);
            }
        }

        void set_frame_timecode(IDeckLinkMutableVideoFrame *frame) {
            uint32_t frames = frames_written;

            uint8_t fr = frames % 30;
            frames /= 30;

            uint8_t sec = frames % 60;
            frames /= 60;

            uint8_t min = frames % 60;
            frames /= 60;

            uint8_t hr = frames;

            frame->SetTimecodeFromComponents(bmdTimecodeVITC, hr, min, 
                sec, fr, bmdTimecodeFlagDefault);
        
            frames_written++;
        }

        void schedule_frame(IDeckLinkMutableVideoFrame *frame) {
            RawFrame *input = NULL;
            void *data;

            if (in_pipe.data_ready( )) {
                input = in_pipe.get( );
            } else if (last_frame != NULL) {
                /* use the stale frame */
                fprintf(stderr, "DeckLink: stale frame\n");
                input = last_frame;
                audio_adjust += 1; /* ugly hack */
            } else {
                /* this should likewise be a black frame */
                input = NULL;
            }
            
            if (input != NULL) {
                frame->GetBytes(&data);
                input->unpack->CbYCrY8422((uint8_t *) data);
            } else {
                fprintf(stderr, "DeckLink: on fire\n");
		audio_adjust += 1;
            }
            
            set_frame_timecode(frame);

            deckLinkOutput->ScheduleVideoFrame(frame, 
                frame_counter * frame_duration, frame_duration, time_base);

            frame_counter++;

            if (last_frame != input) {
                delete last_frame;
                last_frame = input;
            }
        }

        void start_video( ) {
            if (deckLinkOutput->StartScheduledPlayback(0, 100, 1.0) != S_OK) {
                throw std::runtime_error(
                    "Failed to start scheduled playback!\n"
                );
            }
        }

};

class DeckLinkInputAdapter : public InputAdapter, 
        public IDeckLinkInputCallback {

    public:
        DeckLinkInputAdapter(unsigned int card_index = 0,
                unsigned int norm_ = 0, unsigned int input_ = 0,
                RawFrame::PixelFormat pf_ = RawFrame::CbYCrY8422,
                bool enable_audio = false, unsigned int n_channels = 2,
                bool enable_video = true)
                : deckLink(NULL), out_pipe(IN_PIPE_SIZE) {

            audio_pipe = NULL;
            started = false;
            signal_lost = false;
            this->enable_video = enable_video;

            n_frames = 0;
            start_time = 0;
            avsync = 0;

            pf = pf_;
            bpf = convert_pf(pf_);

            deckLink = find_card(card_index);
            select_input_connection(input_);
            open_input(norm_);

            if (enable_audio) {
                audio_pipe = new Pipe<IOAudioPacket *>(IN_PIPE_SIZE);
            }

            this->n_channels = n_channels;
            select_audio_input_connection( );
            open_audio_input( );


            start_capture( );

        }

        ~DeckLinkInputAdapter( ) {
            if (deckLinkInput != NULL) {
                deckLinkInput->Release( );
            }

            if (deckLink != NULL) {
                deckLink->Release( );
            }
        }

        virtual void start( ) {
            started = true;
        }

        virtual HRESULT QueryInterface(REFIID iid, LPVOID *ppv) { 
            UNUSED(iid);
            UNUSED(ppv);
            return E_NOINTERFACE; 
        }

        virtual ULONG AddRef(void) {
            return 1;
        }

        virtual ULONG Release(void) {
            return 1;
        }

        virtual HRESULT VideoInputFormatChanged(
                BMDVideoInputFormatChangedEvents events,
                IDeckLinkDisplayMode *mode,
                BMDDetectedVideoInputFormatFlags flags) {

            UNUSED(events);
            UNUSED(mode);
            UNUSED(flags);

            fprintf(stderr, "DeckLink: input format changed?\n");
            return S_OK;
        }

        virtual HRESULT VideoInputFrameArrived(IDeckLinkVideoInputFrame *in,
                IDeckLinkAudioInputPacket *audio_in) {
            
            RawFrame *out;
            IOAudioPacket *audio_out;

            void *data;

            if (in == NULL && audio_in == NULL) {
                fprintf(stderr, "VideoInputFrameArrived got nothing??\n");
            }

            /* Process video frame if available. */
            if (in != NULL) {
                if (in->GetFlags( ) & bmdFrameHasNoInputSource) {
                    if (!signal_lost) {
                        fprintf(stderr, "DeckLink input: signal lost\n");
                        signal_lost = true;
                    }
                } else {
                    if (signal_lost) {
                        fprintf(stderr, "DeckLink input: signal re-acquired\n");
                        signal_lost = false;
                    }
                }

                /* 
                 * we can't actually tell the card to just not send video,
                 * so if video is disabled, we just ignore the video frames...
                 */
                if (enable_video) {
                    out = create_raw_frame_from_decklink(in, pf);
                    out->set_field_dominance(dominance);
                    
                    if (out_pipe.can_put( ) && started) {
                        if (enable_video) {
                            out_pipe.put(out);
                            avsync++;
                        }
                    } else {
                        fprintf(stderr, "DeckLink: dropping input frame\n");
                        delete out;
                    }
                }
            } 

            /* Process audio, if available. */
            if (audio_in != NULL && audio_pipe != NULL) {
                audio_out = new IOAudioPacket(
                    audio_in->GetSampleFrameCount( ), n_channels
                );

                if (audio_in->GetBytes(&data) != S_OK) {
                    throw std::runtime_error(
                        "DeckLink audio input: GetBytes failed"
                    );
                }

                memcpy(audio_out->data( ), data, audio_out->size_bytes( ));
                if (audio_pipe->can_put( ) && started) {
                    audio_pipe->put(audio_out);
                    avsync--;
                } else {
                    fprintf(stderr, "DeckLink: dropping input AudioPacket\n");
                    delete audio_out;
                }
            } 


            if (
                    (avsync > 10 || avsync < -10) 
                    && audio_pipe != NULL 
                    && enable_video
            ) {
                fprintf(stderr, "DeckLink warning: avsync drift = %d\n", avsync);
            }

            return S_OK;
        }

        virtual Pipe<RawFrame *> &output_pipe( ) {
            return out_pipe;
        }

        virtual Pipe<IOAudioPacket *> *audio_output_pipe( ) { 
            return audio_pipe;
        }

    protected:
        IDeckLink *deckLink;
        IDeckLinkInput *deckLinkInput;
        Pipe<RawFrame *> out_pipe;

        bool started;
        bool signal_lost;
        bool enable_video;

        RawFrame::PixelFormat pf;
        RawFrame::FieldDominance dominance;

        BMDPixelFormat bpf;

        unsigned int audio_rate;
        unsigned int n_channels;

        uint64_t n_frames;
        uint64_t start_time;

        int avsync;

        Pipe<IOAudioPacket *> *audio_pipe;

        void open_input(unsigned int norm) {
            IDeckLinkDisplayModeIterator *it;

            assert(deckLink != NULL);
            assert(norm < sizeof(norms) / sizeof(struct decklink_norm));

            if (deckLink->QueryInterface(IID_IDeckLinkInput,
                    (void **) &deckLinkInput) != S_OK) {
                        
                throw std::runtime_error(
                    "DeckLink input: failed to get IDeckLinkInput"
                );
            }

            if (deckLinkInput->GetDisplayModeIterator(&it) != S_OK) {
                throw std::runtime_error(
                    "DeckLink input: failed to get display mode iterator"
                );
            }
            
            dominance = find_dominance(norms[norm].mode, it);

            it->Release( );

            if (deckLinkInput->EnableVideoInput(norms[norm].mode,
                    bpf, 0) != S_OK) {
                
                throw std::runtime_error(
                    "DeckLink input: failed to enable video input"
                );
            }

            fprintf(stderr, "DeckLink: opening input using norm %s\n",
                    norms[norm].name);
        }

        void select_input_connection(unsigned int input) {
            IDeckLinkConfiguration *config;

            assert(deckLink != NULL);
            assert(input < sizeof(connections) 
                    / sizeof(struct decklink_connection));

            if (deckLink->QueryInterface(IID_IDeckLinkConfiguration, 
                    (void**) &config) != S_OK) {
                
                throw std::runtime_error(
                    "DeckLink input: get IDeckLinkConfiguration failed"
                );
            }

            if (config->SetInt(bmdDeckLinkConfigVideoInputConnection,
                    connections[input].connection) != S_OK) {

                throw std::runtime_error(
                    "DeckLink input: set input connection failed"
                );
            }

            fprintf(stderr, "DeckLink: input connection set to %s\n",
                    connections[input].name);

            config->Release( );
        }   

        void open_audio_input( ) {
            assert(deckLink != NULL);
            assert(deckLinkInput != NULL);

            if (deckLinkInput->EnableAudioInput(bmdAudioSampleRate48kHz,
                    bmdAudioSampleType16bitInteger, n_channels) != S_OK) {
                throw std::runtime_error(
                    "DeckLink audio input: failed to enable audio input"
                );
            }

            audio_rate = 48000;
        }

        void select_audio_input_connection( ) {
            IDeckLinkConfiguration *config;

            assert(deckLink != NULL);

            if (deckLink->QueryInterface(IID_IDeckLinkConfiguration, 
                    (void**) &config) != S_OK) {
                
                throw std::runtime_error(
                    "DeckLink input: get IDeckLinkConfiguration failed"
                );
            }

            if (config->SetInt(bmdDeckLinkConfigAudioInputConnection,
                    bmdAudioConnectionEmbedded) != S_OK) {

                throw std::runtime_error(
                    "DeckLink input: set embedded audio input failed"
                );
            }

            config->Release( );
        }

        void start_capture(void) {
            if (deckLinkInput->SetCallback(this) != S_OK) {
                throw std::runtime_error(
                    "DeckLink input: set callback failed"
                );
            }

            if (deckLinkInput->StartStreams( ) != S_OK) {
                throw std::runtime_error(
                    "DeckLink input: start streams failed"
                );
            }
        }
};


OutputAdapter *create_decklink_output_adapter(unsigned int card_index,
        unsigned int decklink_norm, RawFrame::PixelFormat pf) {

    return new DeckLinkOutputAdapter(card_index, decklink_norm, pf);
}

OutputAdapter *create_decklink_output_adapter_with_audio(
        unsigned int card_index, unsigned int decklink_norm,
        RawFrame::PixelFormat pf, unsigned int n_channels) {
    return new DeckLinkOutputAdapter(card_index, decklink_norm, pf, 
            true, n_channels);
}

InputAdapter *create_decklink_input_adapter(unsigned int card_index,
        unsigned int decklink_norm, unsigned int decklink_input,
        RawFrame::PixelFormat pf) {
    
    return new DeckLinkInputAdapter(card_index, 
            decklink_norm, decklink_input, pf);
}

InputAdapter *create_decklink_input_adapter_with_audio(unsigned int card_index,
        unsigned int decklink_norm, unsigned int decklink_input,
        RawFrame::PixelFormat pf, unsigned int n_channels) {
    
    return new DeckLinkInputAdapter(card_index, 
            decklink_norm, decklink_input, pf, true, n_channels);
}

InputAdapter *create_decklink_audio_input_adapter(
        unsigned int card_index, unsigned int decklink_norm,
        unsigned int decklink_input, unsigned int n_channels
) {
    return new DeckLinkInputAdapter(
        card_index, decklink_norm, decklink_input, 
        RawFrame::CbYCrY8422, true, n_channels, false
    );
}
