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

#include "condition.h"
#include <stdexcept>

Condition::Condition( ) {
    if (pthread_cond_init(&cond, NULL) != 0) {
        throw std::runtime_error("Failed to create pthread condition variable");
    }
}

Condition::~Condition( ) {
    if (pthread_cond_destroy(&cond) != 0) {
        throw std::runtime_error("Failed to destroy condition variable");
    }
}

void Condition::wait(Mutex &mut) {
    if (pthread_cond_wait(&cond, &mut.mut) != 0) {
        throw std::runtime_error("Failed to wait on condition variable");
    }
}

void Condition::signal( ) {
    if (pthread_cond_signal(&cond) != 0) {
        throw std::runtime_error("Failed to signal condition variable");
    }
}

void Condition::broadcast( ) {
    if (pthread_cond_broadcast(&cond) != 0) {
        throw std::runtime_error("Failed to broadcast condition variable");
    }
}

