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
#include "clocks.h"
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
	fprintf(stderr, "warning: failed to destroy mutex\n");
    }
}

void Mutex::lock( ) {
    throw_on_error(pthread_mutex_lock(&mut), "Failed to lock mutex");
    thread_acquired( );
}

void Mutex::unlock( ) {
    thread_released( );
    if (pthread_mutex_unlock(&mut) != 0) {
        throw std::runtime_error("Failed to unlock mutex");
    }
}

void Mutex::thread_acquired( ) {
    msec_locked = clock_monotonic_msec( );
}

void Mutex::thread_released( ) {
    msec_locked = clock_monotonic_msec( ) - msec_locked;

    if (msec_locked > 500) {
        fprintf(stderr, "mutex locked for %lu msec, backtrace follows\n",
            msec_locked);
        print_backtrace( );
    }
}

MutexLock::MutexLock(Mutex &mut) {
    _mut = &mut;
    _mut->lock( );
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
