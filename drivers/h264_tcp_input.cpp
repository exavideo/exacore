/*
 * Copyright 2015 Exavideo LLC.
 * 
 * This file is part of exacore.
 * 
 * exacore is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * exacore is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with exacore.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "adapter.h"

#include <string>
#include <thread>

#include <unistd.h>

extern "C" {
#include <libavcodec/avcodec.h>
}

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define PARSE_BUF_SIZE 1048576

class H264TcpInputAdapter : public InputAdapter {
    public:
        H264TcpInputAdapter(const char *host, const char *port);
        virtual ~H264TcpInputAdapter( );
        virtual Pipe<RawFrame *> &output_pipe( ) { return _out_pipe; }
        virtual void start( );
        virtual void rotate180( ) { /* unimplemented */ }
    
    protected:
        Pipe<RawFrame *> _out_pipe;
        AVCodecContext *codec_ctx;

        int open_socket( );

        void open_codec( );
        void close_codec( );

        std::string _host, _port;
        int sockfd, shutdown;

        std::thread worker;
        bool worker_running;
        void worker_proc( );

        void decode_and_send(uint8_t *data, size_t size);

        AVPacket pkt;
        AVFrame *frame;
};

H264TcpInputAdapter::H264TcpInputAdapter(const char *host, const char *port) 
: _out_pipe(32) {
    _host = host;
    _port = port;
    shutdown = 0;
    sockfd = -1;

    open_codec( );
}

H264TcpInputAdapter::~H264TcpInputAdapter( ) {
    /* shut down the socket */
    shutdown = 1;
    close(sockfd);
    sockfd = -1;

    /* wait for the worker thread to stop */
    if (worker.joinable( )) {
        worker.join( );
    }

    /* close the codec */
    close_codec( );
}

void H264TcpInputAdapter::start( ) {
    /* start the worker thread */
    if (!worker_running) {
        worker_running = true;
        worker = std::thread([this] { worker_proc(); });
    }
}

void H264TcpInputAdapter::open_codec( ) {
    AVCodec *codec;

    /* 
     * make sure H.264 codec is registered 
     *
     * this is not the most efficient way but it should work
     */
    avcodec_register_all( );


    /*
     * initialize a H.264 decode context 
     */
    codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (codec == NULL) {
        throw std::runtime_error("avcodec_find_decoder failed");
    }
    
    codec_ctx = avcodec_alloc_context3(codec);
    if (codec_ctx == NULL) {
        throw std::runtime_error("avcodec_alloc_context3 failed");
    }

    if (avcodec_open2(codec_ctx, codec, NULL) < 0) {
        throw std::runtime_error("avcodec_open2 failed");
    }

    av_init_packet(&pkt);
    frame = av_frame_alloc( );
    if (frame == NULL) {
        throw std::runtime_error("av_frame_alloc failed");
    }
}

void H264TcpInputAdapter::close_codec( ) {
    avcodec_free_context(&codec_ctx);
    codec_ctx = NULL;
}

int H264TcpInputAdapter::open_socket( ) {
    int retval;
    struct addrinfo ai_hints, *ai_results, *ai;

    /* if we already have a socket, close it first */
    if (sockfd != -1) {
        close(sockfd);
        sockfd = -1;
    }

    /* use getaddrinfo() to look up addresses */
    memset(&ai_hints, 0, sizeof(ai_hints));
    ai_hints.ai_family = AF_UNSPEC;
    ai_hints.ai_socktype = SOCK_STREAM;
    ai_hints.ai_flags = AI_ALL | AI_V4MAPPED;
    retval = getaddrinfo(
        _host.c_str(), _port.c_str(), 
        &ai_hints, &ai_results
    );

    if (retval != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(retval));
        return -1;
    }
    
    /* iterate over all found addresses until we find one that works */
    for (ai = ai_results; ai != NULL && sockfd == -1; ai = ai->ai_next) {
        /* attempt to create a socket and connect */
        sockfd = socket(
            ai->ai_family, ai->ai_socktype, ai->ai_protocol
        );
        
        if (sockfd == -1) {
            continue;
        }

        retval = connect(sockfd, ai->ai_addr, ai->ai_addrlen);
        if (retval == -1) {
            close(sockfd);
            sockfd = -1;
        }
    }

    freeaddrinfo(ai_results);

    /* return -1 if connection failed, 0 if it succeeded. */
    if (sockfd == -1) {
        return -1;
    } else {
        return 0;
    }

}

static bool is_nal_start(uint8_t *nal) {
    if (nal[0] == 0x00 && nal[1] == 0x00) {
        if (nal[2] == 0x01 || (nal[2] == 0x00 && nal[3] == 0x01)) {
            return true;
        }
    }
    return false;
}

static int nal_type(uint8_t *nal) {
    if (nal[2] == 0x01) {
        return nal[3] & 0x1f;
    } else {
        return nal[4] & 0x1f;
    }
}

void H264TcpInputAdapter::worker_proc( ) {
    uint8_t *buf;
    int retval;
    size_t last_start = 0;
    size_t bytes_in_buf = 0;
    size_t buf_scanned = 0;

    buf = new uint8_t[PARSE_BUF_SIZE];

    while (shutdown == 0) { 
        /* if the socket is disconnected, try to connect it */
        if (sockfd == -1) {
            if (open_socket( ) == -1) {
                /* if we fail, wait then try again */
                sleep(30);
                continue;
            }

            fprintf(stderr, "socket open (%s:%s)\n", _host.c_str(), _port.c_str());
        }

        /* fill the buffer from the socket */
        retval = read(
            sockfd, buf + bytes_in_buf, 
            PARSE_BUF_SIZE - bytes_in_buf
        );

        if (retval < 0) {
            perror("read");
            close(sockfd);
            sockfd = -1;
            continue;
        } else if (retval == 0) {
            fprintf(stderr, "read EOF on socket\n");
            close(sockfd);
            sockfd = -1;
            continue;
        }

        bytes_in_buf += retval;

        /* scan stream_buf for NAL start sequence */
        while (buf_scanned + 4 < bytes_in_buf) {
            if (is_nal_start(&buf[buf_scanned])) {
                /* 
                 * last_start points at the most recent NAL.
                 * If it's the right type we will decode the entire buffer.
                 */
                switch (nal_type(&buf[last_start])) {
                case 5: /* I-slice, typically preceded by SPS and PPS */
                case 1: /* P-slice */
                    /* decode the entire buffer up to this point */
                    decode_and_send(buf, buf_scanned);

                    /* move any leftover data back to the start */
                    memmove(buf, buf + buf_scanned, bytes_in_buf - buf_scanned);
                    bytes_in_buf -= buf_scanned;
                    buf_scanned = 0;

                    break;
                default:
                    /* let other NAL types accumulate in the buffer */
                    break;
                }
                last_start = buf_scanned;
            }

            buf_scanned++;
        }

    }
}

/* 
 * decode_and_send()
 * 
 * Accept coded frames split up by the worker thread, decode them, and pass 
 * along to the user.
 */
void H264TcpInputAdapter::decode_and_send(uint8_t *data, size_t size) {
    int retval;
    int got_picture;
    RawFrame *output;

    pkt.data = data;
    pkt.size = size;

    retval = avcodec_decode_video2(codec_ctx, frame, &got_picture, &pkt);
    if (retval < 0) {
        fprintf(stderr, "avcodec_decode_video2 failed\n");
        return;
    }

    if (got_picture) {
        if (frame->format == AV_PIX_FMT_YUV420P) {
            output = new RawFrame(
                frame->width, frame->height, RawFrame::CbYCrY8422
            );
            output->pack->YCbCr8P420(
                frame->data[0],
                frame->data[1],
                frame->data[2],
                frame->linesize[0],
                frame->linesize[1],
                frame->linesize[2]
            );
            _out_pipe.put(output);
        }
    }
}

InputAdapter *create_h264_tcp_input_adapter(const char *host, const char *port) {
    return new H264TcpInputAdapter(host, port);
}
