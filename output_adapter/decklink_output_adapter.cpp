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

#include "output_adapter.h"

#include "DeckLinkAPI.h"
#include "types.h"

#include <stdio.h>
#include <assert.h>

struct decklink_norm {
    const char *name;
    BMDTimeScale time_base;
    BMDTimeValue frame_duration;
    BMDDisplayMode mode;
    coord_t w, h;
};

static struct decklink_norm norms[] = {
    { "1080i 59.94", 30000, 1001, bmdModeHD1080i5994, 1920, 1080 },
    { "NTSC", 30000, 1001, bmdModeNTSC, 720, 486 },
};

class DeckLinkOutputAdapter : public OutputAdapter, 
        public IDeckLinkVideoOutputCallback {

    public:
        DeckLinkOutputAdapter(unsigned int card_index = 0, 
                unsigned int norm_ = 0) : deckLink(NULL), 
                deckLinkOutput(NULL), frame_counter(0),
                last_frame(NULL) {

            norm = norm_;
            assert(norm < sizeof(norms) / sizeof(struct decklink_norm));
            time_base = norms[norm].time_base;
            frame_duration = norms[norm].frame_duration;

            deckLink = find_card(card_index);
            open_card( );
            preroll_video_frames(8);
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
        virtual HRESULT QueryInterface(REFIID x, void **y) {
            return E_FAIL;
        }

        virtual ULONG AddRef( ) {
            return 0;
        }

        virtual ULONG Release( ) {
            return 0;
        }

        virtual HRESULT ScheduledPlaybackHasStopped(void) {
            fprintf(stderr, "DeckLink: warning: playback stopped\n");
            return S_OK;
        }

        Pipe<RawFrame *> &input_pipe( ) { return in_pipe; }
    
    protected:        
        IDeckLink *deckLink;
        IDeckLinkOutput *deckLinkOutput;
        
        BMDTimeScale time_base;
        BMDTimeValue frame_duration;
        unsigned int frame_counter;

        unsigned int norm;

        RawFrame *last_frame;

        Pipe<RawFrame *> in_pipe;

        IDeckLink *find_card(int card_index) {
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

        void open_card( ) {
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

            if (deckLinkOutput->EnableVideoOutput(norms[norm].mode, 
                    bmdVideoOutputFlagDefault) != S_OK) {
                
                throw std::runtime_error(
                    "Failed to enable DeckLink video output"
                );
            }
        }

        void preroll_video_frames(unsigned int n_frames) {
            IDeckLinkMutableVideoFrame *frame;
            for (unsigned int i = 0; i < n_frames; i++) {
                if (deckLinkOutput->CreateVideoFrame(norms[norm].w, 
                        norms[norm].h, 2*norms[norm].w, bmdFormat8BitYUV,
                        bmdFrameFlagDefault, &frame) != S_OK) {

                    throw std::runtime_error("Failed to create frame"); 
                }

                schedule_frame(frame);
            }
        }

        void schedule_frame(IDeckLinkMutableVideoFrame *frame) {
            RawFrame *input;
            int ret;
            void *data;

            if (in_pipe.data_ready( )) {
                ret = in_pipe.get(input);
                if (ret < 0) {
                    perror("pipe get");
                    throw std::runtime_error("DeckLink: pipe get failed");
                } else if (ret == 0) {
                    fprintf(stderr, "decklink EOF?");
                    /* FIXME this should be a black frame or something */
                    input = NULL; 
                } /* else it worked and we're OK */
            } else if (last_frame != NULL) {
                /* use the stale frame */
                input = last_frame;
            } else {
                /* this should likewise be a black frame */
                input = NULL;
            }
            
            if (input != NULL) {
                frame->GetBytes(&data);
                input->unpack->CbYCrY8422((uint8_t *) data);
            } else {
                /* no frames... showing something *really* old */
                fprintf(stderr, "DeckLink: on fire\n");
            }
            
            deckLinkOutput->ScheduleVideoFrame(frame, 
                frame_counter * frame_duration, frame_duration, time_base);

            frame_counter++;

            if (last_frame != input) {
                fprintf(stderr, "Deleting frame\n");
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

OutputAdapter *create_decklink_output_adapter(unsigned int card_index,
        unsigned int decklink_norm) {

    return new DeckLinkOutputAdapter(card_index, decklink_norm);
}
