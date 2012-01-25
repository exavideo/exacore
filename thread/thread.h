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

#ifndef _OPENREPLAY_THREAD_H
#define _OPENREPLAY_THREAD_H

#include <pthread.h>

class Thread {
    public:
        Thread( );
        virtual ~Thread( );
        void priority(int scheduler, int priority);

    protected:
        void start_thread(void);
        void join_thread(void);
        virtual void run_thread(void);

        void do_run_thread(void);

        pthread_t pthread;

        /* 
         * Should not be written other than by the constructor 
         * or by do_run_thread 
         */
        bool running; 
       
        static void *thread_proc(void *obj);

};

#endif
