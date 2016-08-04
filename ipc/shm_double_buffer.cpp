/*
 * Copyright (c) 2016 Andrew H. Armenia.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to 
 * deal in the Software without restriction, including without limitation the 
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or 
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 */

#include "shm_double_buffer.h"

#include <unistd.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <stdio.h>
#include <errno.h>

#include <stdexcept>
#include <cassert>

static int memfd_create(const char *name, unsigned int flags) {
	return syscall(SYS_memfd_create, name, flags);
}

ShmDoubleBuffer::ShmDoubleBuffer() {
	fd = -1;
	shared_state = NULL;
	for (int i = 0; i < 2; i++) {
		buffers[i] = NULL;
	}
}

ShmDoubleBuffer::ShmDoubleBuffer(size_t object_size) {
	fd = -1;
	shared_state = NULL;
	for (int i = 0; i < 2; i++) {
		buffers[i] = NULL;
	}

	init(object_size);
}

void ShmDoubleBuffer::begin_read(const void *&buf) {
	assert(shared_state != NULL);
	assert(fd != -1);

	/* 
	 * in the critical section, we determine which buffer to read, and
	 * do a page flip if possible.
	 */
	int bufn;

	lock( );
	assert(
		shared_state->producer_buf == 0 || 
		shared_state->producer_buf == 1
	);
	if (shared_state->ok_to_flip) {
		bufn = shared_state->producer_buf;
		shared_state->producer_buf = 1 - bufn;
		shared_state->ok_to_flip = false;
	} else {
		bufn = 1 - shared_state->producer_buf;
	}
	unlock( );

	buf = buffers[bufn];
	assert(buf != NULL);
}

void ShmDoubleBuffer::end_read() {
	/* no-op for now but here if we need it */
}

void ShmDoubleBuffer::begin_write(void *&buf, bool &was_flipped) {
	int bufn;	
	assert(shared_state != NULL);
	/*
	 * critical section: check if the pages were flipped and set
	 * ok_to_flip to false (if ok_to_flip is ALREADY false then
	 * we know the consumer already did a flip)
	 */
	lock( );
	if (!shared_state->ok_to_flip) {
		was_flipped = true;
	}
	shared_state->ok_to_flip = false;
	bufn = shared_state->producer_buf;
	assert(bufn == 0 || bufn == 1);
	unlock( );

	buf = buffers[bufn];
	assert(buf != NULL);
}

void ShmDoubleBuffer::end_write() {
	lock( );
	assert(shared_state->ok_to_flip == false);
	shared_state->ok_to_flip = true;
	unlock( );
}

void ShmDoubleBuffer::lock() {
	assert(shared_state != NULL);
	while (sem_wait(&shared_state->sem) != 0) {
		/* try again on EAGAIN or EINTR but bail out otherwise */
		if (errno != EAGAIN && errno != EINTR) {
			perror("sem_wait");
			throw std::runtime_error("Failed to lock semaphore");
		}
	}
}

void ShmDoubleBuffer::unlock() {
	assert(shared_state != NULL);
	if (sem_post(&shared_state->sem) != 0) {
		perror("sem_post");
		throw std::runtime_error("Failed to unlock semaphore");
	}
}

void ShmDoubleBuffer::init(size_t bufsize) {
	int n_pages_per_buffer;
	size_t page_size = sysconf(_SC_PAGESIZE);
	size_t n_bytes;

	assert(bufsize > 0);

	/* create the file with the correct size */
	if (page_size < sizeof(struct state)) {
		throw std::runtime_error("struct state doesn't fit one page");
	}

	/* 
	 * FIXME: this wastes a page if bufsize is a multiple of page_size 
	 * (note this calculation is repeated in map_buffers() )
	 */
	n_pages_per_buffer = (bufsize / page_size) + 1;
	n_bytes = (2 * n_pages_per_buffer + 1) * page_size;

	fd = memfd_create("ShmDoubleBuffer", 0);
	if (fd == -1) {
		perror("memfd_create");
		throw std::runtime_error("memfd_create failed");
	}
	
	if (ftruncate(fd, n_bytes) != 0) {
		perror("ftruncate");
		throw std::runtime_error("ftruncate failed");
	}

	/* map in the state */
	map_state( );

	/* initialize the state */
	shared_state->ok_to_flip = false;
	shared_state->producer_buf = 0;
	shared_state->bufsize = bufsize;	
	if (sem_init(&shared_state->sem, 1, 1) != 0) {
		throw std::runtime_error("sem_init failed");
	}

	/* now we can map in the buffers */
	map_buffers( );
}

void ShmDoubleBuffer::init_from_fd(int nfd) {
	if (fd != -1) {
		throw std::runtime_error("tried to re-init ShmDoubleBuffer");
	}

	fd = nfd;
	/* 
	 * all we need to do is map in the state, then map the buffers, 
	 * because the state is already initialized.
	 */
	map_state( );
	map_buffers( );
}

void ShmDoubleBuffer::map_state( ) {
	assert(fd != -1);
	assert(shared_state == NULL);

	/* map in one page for state */
	size_t page_size = sysconf(_SC_PAGESIZE);

	void *ret = mmap(
		NULL, page_size, PROT_READ | PROT_WRITE, 
		MAP_SHARED, fd, 0
	);

	if (ret == MAP_FAILED) {
		perror("mmap");
		throw std::runtime_error("failed to map in shared state");
	}

	shared_state = (struct state *)ret;
}

void ShmDoubleBuffer::map_buffers( ) {
	assert(fd != -1);
	assert(shared_state != NULL);

	size_t page_size = sysconf(_SC_PAGESIZE);
	size_t npages = (shared_state->bufsize / page_size) + 1;
	size_t start_page;
	off_t start;
	size_t length = npages * page_size;


	for (int i = 0; i < 2; i++) {
		start_page = (npages * i) + 1;
		start = start_page * page_size;
		assert(buffers[i] == NULL);

		buffers[i] = mmap(
			NULL, length, PROT_READ | PROT_WRITE,
			MAP_SHARED, fd, start
		);

		if (buffers[i] == MAP_FAILED) {
			perror("mmap");
			buffers[i] = NULL;
			throw std::runtime_error("failed to map in buffer");
		}
	}
}

int ShmDoubleBuffer::get_fd() {
	assert(fd != -1);
	return fd;
}
