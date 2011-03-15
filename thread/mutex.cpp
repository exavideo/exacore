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
    if (pthread_mutex_lock(&mut) != 0) {
        throw std::runtime_error("Failed to lock mutex");
    }
}

void Mutex::unlock( ) {
    if (pthread_mutex_unlock(&mut) != 0) {
        throw std::runtime_error("Failed to unlock mutex");
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
