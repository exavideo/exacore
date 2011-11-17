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

#include "adapter.h"

#include "DeckLinkAPI.h"
#include "types.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>

#define IN_PIPE_SIZE 32
#define OUT_PIPE_SIZE 32 

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

class DeckLinkOutputAdapter : public OutputAdapter, 
        public IDeckLinkVideoOutputCallback,
        public IDeckLinkAudioOutputCallback {

    public:
        DeckLinkOutputAdapter(unsigned int card_index = 0, 
                unsigned int norm_ = 0, 
                RawFrame::PixelFormat pf_ = RawFrame::CbYCrY8422,
                bool enable_audio = false) 
                : deckLink(NULL), 
                deckLinkOutput(NULL), frame_counter(0),
                last_frame(NULL), in_pipe(OUT_PIPE_SIZE), audio_in_pipe(NULL) {

            norm = norm_;
            assert(norm < sizeof(norms) / sizeof(struct decklink_norm));
            time_base = norms[norm].time_base;
            frame_duration = norms[norm].frame_duration;

            pf = pf_;
            bpf = convert_pf(pf_); 

            deckLink = find_card(card_index);
            configure_card( );
            open_card( );
            preroll_video_frames(4);

            if (enable_audio) {
                setup_audio( );
            }

            start_video( );

            fprintf(stderr, "DeckLink: initialized using norm %s\n", 
                    norms[norm].name);
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
            if (preroll) {
                if (current_audio_pkt != NULL) {
                    try_finish_current_audio_packet( );
                    if (current_audio_pkt == NULL) {
                        audio_preroll_done = 1;
                    }
                } 
            } else {
                /* Cram as much audio into the buffer as we can. */
                for (;;) {
                    if (current_audio_pkt != NULL) {
                        if (try_finish_current_audio_packet( ) == 0) {
                            /* 
                             * if we are writing nothing... the buffer must
                             * be full. Stop now.
                             */
                            break;
                        }
                    } else if (audio_in_pipe->data_ready( )) {
                        current_audio_pkt = audio_in_pipe->get( );
                        samples_written_from_current_audio_pkt = 0;
                    } else {
                        /* 
                         * current audio packet is NULL.
                         * Nothing is in the pipe.
                         * Stop now.
                         */
                        break;
                    }
                }
            }

            return S_OK;
        }

        Pipe<RawFrame *> &input_pipe( ) { return in_pipe; }
        Pipe<AudioPacket *> *audio_input_pipe( ) { return audio_in_pipe; }

        RawFrame::FieldDominance output_dominance( ) { return dominance; }
    
    protected:        
        IDeckLink *deckLink;
        IDeckLinkOutput *deckLinkOutput;
        
        BMDTimeScale time_base;
        BMDTimeValue frame_duration;
        unsigned int frame_counter;

        unsigned int norm;

        RawFrame *last_frame;

        Pipe<RawFrame *> in_pipe;

        RawFrame::PixelFormat pf;
        RawFrame::FieldDominance dominance;
        BMDPixelFormat bpf;

        volatile int audio_preroll_done;
        unsigned int n_channels;
        Pipe<AudioPacket *> *audio_in_pipe;

        AudioPacket *current_audio_pkt;
        uint32_t samples_written_from_current_audio_pkt;

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
            /* FIXME hard coded default */
            n_channels = 2; 

            audio_in_pipe = new Pipe<AudioPacket *>(OUT_PIPE_SIZE);

            /* FIXME magic 29.97 related number */
            /* Set up empty audio packet for prerolling */
            current_audio_pkt = new AudioPacket(48000, n_channels, 2, 6404);
            samples_written_from_current_audio_pkt = 0;

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
            for (unsigned int i = 0; i < n_frames; i++) {
                if (deckLinkOutput->CreateVideoFrame(norms[norm].w, 
                        norms[norm].h, 2*norms[norm].w, bpf,
                        bmdFrameFlagDefault, &frame) != S_OK) {

                    throw std::runtime_error("Failed to create frame"); 
                }

                schedule_frame(frame);
            }
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
            } else {
                /* this should likewise be a black frame */
                input = NULL;
            }
            
            if (input != NULL) {
                frame->GetBytes(&data);
                input->unpack->CbYCrY8422((uint8_t *) data);
            } else {
                fprintf(stderr, "DeckLink: on fire\n");
            }
            
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

        /* 
         * Try sending some audio data to the Decklink.
         * Call only with current_audio_pkt != NULL.
         * Will set current_audio_pkt to NULL and delete the packet
         * if it is fully consumed.
         * Return number of samples consumed.
         */
        int try_finish_current_audio_packet( ) {
            uint32_t n_consumed;
            uint32_t n_left;

            assert(current_audio_pkt != NULL);

            n_left = current_audio_pkt->n_frames( ) 
                    - samples_written_from_current_audio_pkt;

            if (deckLinkOutput->ScheduleAudioSamples(
                    current_audio_pkt->sample(
                        samples_written_from_current_audio_pkt
                    ), n_left, 0, 0, &n_consumed) != S_OK) {
                throw std::runtime_error(
                    "Failed to schedule audio samples"
                );
            }

            if (n_consumed == n_left) {
                delete current_audio_pkt;
                current_audio_pkt = NULL;
            } else if (n_consumed < n_left) {
                samples_written_from_current_audio_pkt += n_consumed;
            } else {
                throw std::runtime_error("This should not happen");
            }

            return n_consumed;
        }
};

class DeckLinkInputAdapter : public InputAdapter, 
        public IDeckLinkInputCallback {

    public:
        DeckLinkInputAdapter(unsigned int card_index = 0,
                unsigned int norm_ = 0, unsigned int input_ = 0,
                RawFrame::PixelFormat pf_ = RawFrame::CbYCrY8422,
                bool enable_audio = false) 
                : deckLink(NULL), out_pipe(IN_PIPE_SIZE) {

            audio_pipe = NULL;

            pf = pf_;
            bpf = convert_pf(pf_);

            deckLink = find_card(card_index);
            select_input_connection(input_);
            open_input(norm_);

            if (enable_audio) {
                n_channels = 2;
                select_audio_input_connection( );
                open_audio_input( );
            }

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
            AudioPacket *audio_out;
            void *data;
            uint8_t *bytes;
            size_t i, spitch, h;

            if (in != NULL) {
                if (in->GetFlags( ) & bmdFrameHasNoInputSource) {
                    fprintf(stderr, "DeckLink input: no signal\n");
                } else {
                    out = new RawFrame(in->GetWidth(), in->GetHeight(), pf);
                    out->set_field_dominance(dominance);
                    spitch = in->GetRowBytes( );
                    h = in->GetHeight( );
                    in->GetBytes(&data);

                    bytes = (uint8_t *) data;

                    for (i = 0; i < h; i++) {
                        memcpy(out->scanline(i), bytes, out->pitch( ));
                        bytes += spitch;
                    }
                    
                    if (out_pipe.can_put( )) {
                        out_pipe.put(out);
                    } else {
                        fprintf(stderr, "DeckLink: dropping input frame on floor\n");
                        delete out;
                    }
                }
            }

            if (audio_in != NULL && audio_pipe != NULL) {
                audio_out = new AudioPacket(audio_rate, n_channels, 2, 
                        audio_in->GetSampleFrameCount( ));

                if (audio_in->GetBytes(&data) != S_OK) {
                    throw std::runtime_error(
                        "DeckLink audio input: GetBytes failed"
                    );
                }

                memcpy(audio_out->data( ), data, audio_out->size( ));
                audio_pipe->put(audio_out);
            }

            return S_OK;
        }

        virtual Pipe<RawFrame *> &output_pipe( ) {
            return out_pipe;
        }

        virtual Pipe<AudioPacket *> *audio_output_pipe( ) { 
            return audio_pipe;
        }

    protected:
        IDeckLink *deckLink;
        IDeckLinkInput *deckLinkInput;
        Pipe<RawFrame *> out_pipe;

        RawFrame::PixelFormat pf;
        RawFrame::FieldDominance dominance;

        BMDPixelFormat bpf;

        unsigned int audio_rate;
        unsigned int n_channels;

        Pipe<AudioPacket *> *audio_pipe;

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

            audio_pipe = new Pipe<AudioPacket *>(IN_PIPE_SIZE);

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
        RawFrame::PixelFormat pf) {
    return new DeckLinkOutputAdapter(card_index, decklink_norm, pf, true);
}

InputAdapter *create_decklink_input_adapter(unsigned int card_index,
        unsigned int decklink_norm, unsigned int decklink_input,
        RawFrame::PixelFormat pf) {
    
    return new DeckLinkInputAdapter(card_index, 
            decklink_norm, decklink_input, pf);
}

InputAdapter *create_decklink_input_adapter_with_audio(unsigned int card_index,
        unsigned int decklink_norm, unsigned int decklink_input,
        RawFrame::PixelFormat pf) {
    
    return new DeckLinkInputAdapter(card_index, 
            decklink_norm, decklink_input, pf, true);
}

