#include <sched.h>
#include <pthread.h>
#include <stdio.h>

#include <linux/futex.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/syscall.h>

#include "pipe.h"

int futex(volatile int *uaddr, int op, int val, const struct timespec *timeout,
        int *uaddr2, int val3) {
    return syscall(SYS_futex, uaddr, op, val, timeout, uaddr2, val3);
}

/* Not fully lock/wait-free but maybe faster than pthreads under load? */
template <class T>
class PipeLF {
    public:
        PipeLF(unsigned int size_) {
            read_ptr = write_ptr = 0;
            array = new T[size_];
            size = size_;
            read_done = false;
            write_done = false;
        }

        ~PipeLF( ) {
            delete [] array;
        }

        T get( ) {
            T ret;
            for (;;) {
                if (read_ptr == write_ptr) {
                    if (write_done) {
                        throw BrokenPipe( );
                    }
                    sched_yield( );
                    continue;
                } else {
                    ret = array[read_ptr];
                    read_ptr = next(read_ptr);
                    return ret;
                }
            }
        }

        void put(const T& input) {
            for (;;) {
                int next_write_ptr = next(write_ptr);
                if (next_write_ptr == read_ptr) {
                    if (read_done) {
                        throw BrokenPipe( );
                    }
                    sched_yield( );
                    continue;
                } else {
                    array[write_ptr] = input;
                    write_ptr = next_write_ptr;
                    return;
                }
            }
        }

        void done_reading(void) {
            read_done = true;
        }

        void done_writing(void) {
            write_done = true;
        }
    protected:
        int next(int i) {
            return (i + 1) % size;
        }
        volatile int read_ptr, write_ptr;
        volatile T *array;
        unsigned int size;

        volatile bool read_done, write_done;
};

PipeLF<int> aPipe(16);

void *thread(void *arg) {
    (void) arg;

    for (;;) {
        try {
            int val = aPipe.get( );
            if (val == 5000) {
                aPipe.done_reading( );
            }
            fprintf(stderr, "%d\n", val);
        } catch (BrokenPipe &ex) {
            fprintf(stderr, "Done!\n");
            return NULL;
        }
    }
}

int main( ) {
    pthread_t pth;
    pthread_create(&pth, NULL, thread, NULL);

    for (int i = 1; i < 10000; i++) {
        aPipe.put(i);
    }

    aPipe.done_writing( );

    pthread_join(pth, NULL);

    return 0;

}
