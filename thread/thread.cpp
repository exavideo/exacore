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

#include "thread.h"
#include <stdexcept>
#include <stdio.h>

Thread::Thread( ) {
    running = false;
}

Thread::~Thread( ) {
    /* nothing to do */
}

void Thread::start_thread(void) {
    if (pthread_create(&pthread, NULL, thread_proc, (void *) this) != 0) {
        throw std::runtime_error("pthread_create failed");
    } 

    fprintf(stderr, "new thread %d\n", (int) pthread);
}

void Thread::priority(int policy, int priority) {
    struct sched_param param;
    param.sched_priority = priority;

    pthread_setschedparam(pthread, policy, &param);
}

void Thread::join_thread(void) {
    if (pthread_join(pthread, NULL) != 0) {
        throw std::runtime_error("pthread_join failed");
    }
}

void Thread::run_thread(void) {
    /* main body of the thread. This will be overridden by a derived class. */
}

void Thread::do_run_thread(void) {
    running = true;
    run_thread( );
    running = false;
}

void *Thread::thread_proc(void *obj) {
    Thread *threadp = (Thread *) obj;

    threadp->do_run_thread( );

    return NULL;
}
