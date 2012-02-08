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

#include "mutex.h"
#include <pthread.h>
#include <stdexcept>

#include "posix_util.h"

#include <sys/time.h>
#include <stdio.h>
#include "backtrace.h"

static void throw_on_error(int ret, const char *msg) {
    if (ret != 0) {
        throw POSIXError(msg, ret);
    }
}

Mutex::Mutex( ) {
    pthread_mutexattr_t attr;

    if (pthread_mutexattr_init(&attr) != 0) {
        throw std::runtime_error("Failed to initialize mutex attribute");
    }

    if (pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE) != 0) {
        throw std::runtime_error("Failed to set mutex as recursive");
    }

    if (pthread_mutex_init(&mut, &attr) != 0) {
        throw std::runtime_error("Failed to initialize mutex");
    }

    pthread_mutexattr_destroy(&attr);
}

Mutex::~Mutex( ) {
    if (pthread_mutex_destroy(&mut) != 0) {
        throw std::runtime_error("Failed to destroy mutex");
    }
}

void Mutex::lock( ) {
    throw_on_error(pthread_mutex_lock(&mut), "Failed to lock mutex");
}

void Mutex::unlock( ) {
    if (pthread_mutex_unlock(&mut) != 0) {
        throw std::runtime_error("Failed to unlock mutex");
    }
}

MutexLock::MutexLock(Mutex &mut) {
    /* instrumented to determine if we're having lock performance issues */
    struct timeval tv1, tv2;
    _mut = &mut;

    gettimeofday(&tv1, NULL);
    _mut->lock( );
    gettimeofday(&tv2, NULL);

    /* subtract time values tv2-tv1 */
    if (tv1.tv_usec > tv2.tv_usec) {
        /* borrow if necessary */
        tv2.tv_sec -= 1;
        tv2.tv_usec += 1000000;
    }

    tv2.tv_sec -= tv1.tv_sec;
    tv2.tv_usec -= tv1.tv_usec;

    if (tv2.tv_sec > 1 || tv2.tv_usec > 20000) {
        fprintf(stderr, "mutex %p blocked for %d.%06d sec\n", 
            _mut, (int) tv2.tv_sec, (int) tv2.tv_usec);
        print_backtrace( );
    }
    
    locked = true;
}

void MutexLock::force_unlock( ) {
    _mut->unlock( );
    locked = false;
}

MutexLock::~MutexLock( ) {
    if (locked) {
        _mut->unlock( );
    }
}
