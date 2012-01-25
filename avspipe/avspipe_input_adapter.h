/*
 * Copyright 2011 Exavideo LLC.
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

#ifndef _AVSPIPE_INPUT_ADAPTER_H
#define _AVSPIPE_INPUT_ADAPTER_H

#include "raw_frame.h"
#include "audio_packet.h"
#include "avspipe_reader_thread.h"
#include "avspipe_allocators.h"

class AvspipeInputAdapter {
    public:
        AvspipeInputAdapter(const char *cmd, bool use_builtin_audio = false);
        ~AvspipeInputAdapter( );
        Pipe<RawFrame *> &output_pipe( );
        Pipe<AudioPacket *> *audio_output_pipe( );
    protected:
        char *parse_command(const char *cmd, int vpfd, int apfd);
        pid_t start_subprocess(const char *cmd, int &vpfd, int &apfd);
        pid_t start_aplay(int apfd);

        Pipe<RawFrame *> vpipe;
        Pipe<AudioPacket *> *apipe;

        AvspipeReaderThread<RawFrame, AvspipeRawFrame1080Allocator> *vread;
        AvspipeReaderThread<AudioPacket, AvspipeNTSCSyncAudioAllocator> 
            *aread;

        int vpfd, apfd;
};

#endif
