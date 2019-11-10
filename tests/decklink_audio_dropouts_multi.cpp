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
#include "thread.h"
#include "pipe.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void test_card(int card) {
    RawFrame *frame = NULL;
    IOAudioPacket *apkt = NULL;

    InputAdapter *iadp;
    Pipe<IOAudioPacket *> *apipe;

    iadp = create_decklink_input_adapter_with_audio(card, 0, 0, RawFrame::CbYCrY8422);
    apipe = iadp->audio_output_pipe( );
    iadp->start();

    size_t vframes = 0, aframes = 0, ok_aframes = 0;

    for (;;) {
        if (iadp->output_pipe( ).data_ready( )) {
            frame = iadp->output_pipe().get();
            vframes++;
            delete frame;
            frame = NULL;
        } else {
            apkt = apipe->get();
            aframes++;
            int16_t *data = apkt->data();
            size_t n = apkt->size_words();
            for (size_t i = 0; i < n; i++) {
                if (data[i] != 0) {
                    ok_aframes++;
                    break;
                }
            }
            delete apkt;
            apkt = NULL;

            if (aframes % 300 == 0) {
                fprintf(stderr, "[card %d] aframes=%6zu  ok=%6zu  bad=%6zu\n",
                    card, aframes, ok_aframes, aframes - ok_aframes);
            }
        }
    }
}

class TestThread : public Thread {
    public:
        TestThread(int card) : Thread() {
            card_ = card;
            start_thread();
        }
        void wait() {
            join_thread();
        }
    protected:
        void run_thread() {
            test_card(card_);
        }
        int card_;
};

int main() {
    TestThread *threads[4];
    for (int i = 0; i < 4; i++) {
        threads[i] = new TestThread(i);
    }
    for (int i = 0; i < 4; i++) {
        threads[i]->wait();
    }
}
