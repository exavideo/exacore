/*
 * Copyright 2011 Exavideo LLC.
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

#include "replay_mjpeg_ingest.h"
#include "mjpeg_codec.h"
#include <assert.h>
#include <string.h>

#define BUFSIZE 1048576

ReplayMjpegIngest::ReplayMjpegIngest(const char *cmd, 
        ReplayBuffer *buf_) {
    int jpeg_pipefd[2];
    int cmd_pipefd[2];

    buf = buf_;
    iadp = NULL;

    /* make a pipe for the mjpeg data */
    if (pipe(jpeg_pipefd) < 0) {
        throw POSIXError("ReplayMjpegIngest pipe()");
    }

    /* and another one for command output */
    if (pipe(cmd_pipefd) < 0) {
        throw POSIXError("ReplayMjpegIngest pipe()");
    }

    /* fork subprocess */
    child_pid = fork( );
    if (child_pid < 0) {
        throw POSIXError("ReplayMjpegIngest fork()");
    } else if (child_pid == 0) {
        close(jpeg_pipefd[0]);
        close(cmd_pipefd[1]);
        dup2(cmd_pipefd[0], STDIN_FILENO);
        dup2(jpeg_pipefd[1], STDOUT_FILENO);
        execlp("sh", "sh", "-c", cmd, NULL);
        /* we should never get here */
        perror("execlp");
        abort( );
    } else {
        close(jpeg_pipefd[1]);
        jpeg_fd = jpeg_pipefd[0];

        close(cmd_pipefd[0]);
        cmd_fd = cmd_pipefd[1];

        jpegbuf = new uint8_t[BUFSIZE];
        buf_size = BUFSIZE;
        buf_fill = 0;

        start_thread( );
    }
}

ReplayMjpegIngest::~ReplayMjpegIngest( ) {
    close(jpeg_fd);
    close(cmd_fd);
}

void ReplayMjpegIngest::run_thread( ) {
    Mjpeg422Decoder dec(1920, 1080);
    Mjpeg422Encoder thm_enc(1920, 1080);
    ReplayRawFrame *monitor_frame;
    ReplayFrameData dest;
    RawFrame *decoded_monitor;

    for (;;) {
        /* obtain writable frame from buffer */
        buf->get_writable_frame(dest);

        /* read M-JPEG data from child process */
        if (read_mjpeg_data(dest) == 0) {
            break; /* no more JPEG data */
        }

        /* set field dominance if necessary */
        if (buf->field_dominance( ) == RawFrame::UNKNOWN) {
            /* assume progressive since we don't know what is coming in */
            buf->set_field_dominance(RawFrame::PROGRESSIVE);
        }

        /* decode JPEG for monitor */
        decoded_monitor = dec.decode(dest.main_jpeg( ), 
                dest.main_jpeg_size( ), 480);
        
        /* encode thumbnail */
        thm_enc.encode_to(decoded_monitor, dest.thumb_jpeg( ),
                dest.thumb_jpeg_size( ));

        buf->finish_frame_write(dest);

        /* scale down frame to send to monitor */
        monitor_frame = new ReplayRawFrame(decoded_monitor);
        
        /* fill in monitor status info */
        monitor_frame->source_name = buf->get_name( );
        monitor_frame->tc = dest.pos;

        monitor.put(monitor_frame);
    }
}

int ReplayMjpegIngest::read_mjpeg_data(ReplayFrameData &dest) {
    ssize_t ret;
    size_t i, jpgend, jpgstart;
    bool success_flag;

    for (;;) {
        /* try to read more data into the buffer */
        if (buf_fill < buf_size) {
            ret = read(jpeg_fd, jpegbuf + buf_fill, buf_size - buf_fill);
            if (ret < 0) {
                throw POSIXError("ReplayMjpegIngest read()");
            } else if (ret == 0) {
                return 0;
            }
            buf_fill += ret;
        }

        jpgstart = (size_t) -1;

        /* scan the buffer for a JPEG end marker (0xffd9) */
        for (i = 0; i < buf_fill - 1; i++) {
            if (jpegbuf[i] == 0xff && jpegbuf[i+1] == 0xd8) {
                jpgstart = i;
            }

            if (jpegbuf[i] == 0xff && jpegbuf[i+1] == 0xd9) {
                jpgend = i+2;
                
                success_flag = false;

                if (jpgstart != (size_t) -1) {
                    /* copy JPEG data to frame buffer */
                    memcpy(dest.main_jpeg( ), jpegbuf+jpgstart, 
                            jpgend-jpgstart);
                    success_flag = true;
                }

                /* move down data following the JPEG (or just discard data) */
                memmove(jpegbuf, jpegbuf+jpgend, buf_fill-jpgend);
                buf_fill -= jpgend;
            
                /* done! */
                if (success_flag) {
                    return 1;
                } else {
                    /* stop scanning for JPEGs and load buffer again */
                    break; 
                }
            }
        }

        if (buf_fill == buf_size) {
            /* 
             * if we've filled the whole buffer, and haven't found a JPEG...
             * something has gone very wrong. Exit.
             */
            throw std::runtime_error("data stream appears not to be JPEG");
        }
    }
}

void ReplayMjpegIngest::trigger( ) {
    const char *cmd = "DUMP\n";
    write_all(cmd_fd, cmd, strlen(cmd));
}
