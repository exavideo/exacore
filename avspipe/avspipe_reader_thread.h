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

#ifndef _AVSPIPE_READER_THREAD_H
#define _AVSPIPE_READER_THREAD_H

#include "thread.h"
#include "pipe.h"

#include <typeinfo>

template <class SendableThing, class _Allocator>
class AvspipeReaderThread : public Thread {
    public:
        AvspipeReaderThread(Pipe<SendableThing *> *fpipe, int in_fd) {
            assert(fpipe != NULL);
            assert(in_fd >= 0);

            _fpipe = fpipe;
            _in_fd = in_fd;

            start_thread( );
        }

        void join( ) {
            join_thread( );
        }
    protected:
        void run_thread(void) {
            SendableThing *thing;
            for (;;) {
                thing = _allocator.allocate( );

                if (thing->read_from_fd(_in_fd) == 0) {
                    _fpipe->done_writing( );
                    break;
                }

                try {
                    _fpipe->put(thing); 
                } catch (BrokenPipe &ex) {
                    break;
                }
            }

            close(_in_fd);
        }

        Pipe<SendableThing *> *_fpipe;
        int _in_fd;
        _Allocator _allocator;
};

#endif
