#ifndef _SHM_DOUBLE_BUFFER_H
#define _SHM_DOUBLE_BUFFER_H

#include <cstddef>
#include <semaphore.h>

/*
 * A double-buffered shared memory region.
 * 
 * Manages two buffers of a specified size, shared between a consumer process
 * and a producer process. Each process is able to quickly lock one of the
 * buffers. However, the consumer and producer will never lock the same 
 * buffer. The consumer always "sees" the latest version possible of the
 * buffer; the producer is notified when its buffer has been swapped.
 */
class ShmDoubleBuffer {
	public:
		ShmDoubleBuffer();
		explicit ShmDoubleBuffer(size_t object_size);	

		/*
		 * Don't construct a new buffer, but use the fd given
		 * to access an existing buffer. Useful for passing
		 * buffers around to child processes.
		 *
		 * Use the default constructor, then your_buf.init_from_fd().
		 */
		void init_from_fd(int fd);

		/*
		 * Get the fd associated with this buffer. It is an error to
		 * call this function on a buffer that isn't initialized
		 * (either via the constructor with size or via init_from_fd)
		 */
		int get_fd();

		/* consumer API */

		/* 
		 * begin_read() flips the buffers (if possible) and 
		 * always returns a readable buffer.
		 */
		void begin_read(const void *&buf);
		/*
		 * The consumer must call end_read() when reading is
		 * finished, though currently the function is a no-op.
		 */
		void end_read();

		/* producer API */

		/*
		 * Lock one of the two buffers for writing and signal
		 * the consumer that the pages may not be flipped. If
		 * the pages were flipped since the last call to begin_write,
		 * was_flipped is set to true.
		 */
		void begin_write(void *&buf, bool &was_flipped);
		/*
		 * Unlock the buffer obtained by begin_write and signal
		 * the consumer that it is now OK to flip pages.
		 */
		void end_write();

	private:
		/* 
		 * the first page of the fd will be the state
		 * subsequent pages will be the buffers
		 */
		int fd;

		void *buffers[2];
		struct state {
			sem_t sem;
			bool ok_to_flip;
			/* 
			 * note that the consumer_buf can always be obtained 
			 * from 1 - producer_buf or !producer_buf
			 */
			int producer_buf;

			/* immutable once the double buffer is created */
			size_t bufsize;
		} *shared_state;

		void lock();
		void unlock();
		void init(size_t bufsize);
		void map_state();
		void map_buffers();
};

#endif
