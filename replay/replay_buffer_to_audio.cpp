/*
 * Copyright 2013 Exavideo LLC.
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

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "posix_util.h"
#include "ajfft.h"
#include "block_set.h"

#define REPLAY_PVOC_BLOCK "ReplPvoc"

void process(int input_fd, int output_fd);

int main(int argc, const char **argv) {
    int input_fd, output_fd;

    if (argc != 3) {
        fprintf(stderr, "usage: replay_buffer_to_audio <replay_buffer> <raw_pcm_file>");
    }

    /* input must be seekable - i.e. a file */
    input_fd = open(argv[1], O_RDONLY);
    if (input_fd < 0) {
        fprintf(stderr, "cannot open buffer file %s: %s\n",
            argv[1], strerror(errno));
        exit(1);
    }

    /* output can be to stdout or file */
    if (!strcmp(argv[2], "-")) {
        output_fd = STDOUT_FILENO;
    } else {
        output_fd = open(argv[2], O_RDWR | O_CREAT | O_TRUNC, 0644);
        if (output_fd < 0) {
            fprintf(stderr, "cannot open output file %s: %s\n",
                argv[2], strerror(errno));
            exit(1);
        }
    }

    process(input_fd, output_fd);

    return 0;
}

void process(int input_fd, int output_fd) {
    off_t current_offset = 0;
    std::complex<float> *frame_data = NULL;
    std::complex<float> *last_frame_data = NULL;

    size_t count = 0;
    size_t frame_count = 0, hop_factor = 8;

    FFT<float> *ifft = NULL;
    std::complex<float> *ifft_result = NULL;
    int16_t *samples = NULL;
    float scale_factor = 1.0;

    for (;;) {
        BlockSet blkset;
        try {
            blkset.begin_read(input_fd, current_offset);
            delete [] last_frame_data;
            last_frame_data = frame_data;
            frame_data = blkset.load_alloc_block<std::complex<float> >(REPLAY_PVOC_BLOCK, count);

            if (ifft == NULL) {
                ifft = new FFT<float>(count, FFT<float>::INVERSE);
                ifft_result = new std::complex<float>[count];
                samples = new int16_t[count];
                scale_factor = float(count) * float(hop_factor);
            }

        } catch (...) {
            /* 
             * if it bombs out we are probably past end of file but who knows
             * so we just close the output_fd and rethrow. This is really ugly
             * but will make sure that any really bad errors get printed
             * rather than ignored.
             */
            close(input_fd);
            close(output_fd);
            throw;
        }

        /* 
         * phase information is stored as a delta from last_frame_data.
         * If last_frame_data is not NULL, we add the phase values into
         * those in frame_data.
         */
        if (last_frame_data != NULL) {
            for (size_t i = 0; i < count; i++) {
                frame_data[i] = std::polar(
                    std::abs(frame_data[i]),
                    std::arg(frame_data[i]) + std::arg(last_frame_data[i])
                );
            }
        }

        /* 
         * now if frame_count % hop_factor == 0, take the IFFT.
         * This should yield a block of (approximately) original
         * audio data.
         */
        if (frame_count % hop_factor == 0) {
            ifft->compute(ifft_result, frame_data); 

            /* take real part and scale as needed */
            for (size_t i = 0; i < count; i++) {
                samples[i] = std::real(ifft_result[i]) / scale_factor;
            }

            write_all(output_fd, samples, count * sizeof(*samples));
        }

        frame_count++;
        current_offset = blkset.end_offset( );
    }
}
