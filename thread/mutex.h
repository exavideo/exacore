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

#ifndef _MUTEX_H
#define _MUTEX_H

#include <pthread.h>

class Condition;

class Mutex {
    public:
        Mutex( );
        ~Mutex( );
        void lock( );
        void unlock( );
    protected:
        pthread_mutex_t mut;
        friend class Condition;
};

class MutexLock {
    public:
        MutexLock(Mutex &mut);
        void force_unlock( );
        ~MutexLock( );
    protected:
        Mutex *_mut;
        bool locked;
};

#endif

