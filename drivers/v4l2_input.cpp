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

#include "adapter.h"
#include "v4l2_input.h"
#include "posix_util.h"
#include "thread.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <string.h>

/* upscale a 640 pixel scanline to 1920 */
static void upscale_scanline(uint8_t *dst_scanline, uint8_t *src_scanline) {
    const coord_t offset = 0; /* offset within a scanline larger than 640 */
    uint16_t y0, y1, cb0, cb1, cr0, cr1;
    /* copy luma values */
    for (coord_t i = 0; i < 640; i++) {
        /* luma */
        dst_scanline[6*i + 1] = src_scanline[2*(i + offset) + 1];
    }

    /* copy chroma values */
    for (coord_t i = 0; i < 320; i++) {
        dst_scanline[12*i + 0] = src_scanline[4*i + 2*offset]; /* Cb */
        dst_scanline[12*i + 2] = src_scanline[4*i + 2*offset + 2]; /* Cr */
    }

    /* luma interpolation */
    for (coord_t i = 0; i < 639; i++) {
        y0 = dst_scanline[6*i + 1];
        y1 = dst_scanline[6*i + 7];
        dst_scanline[6*i + 3] = (2 * y0 + y1) / 3;
        dst_scanline[6*i + 5] = (2 * y1 + y0) / 3;
    }

    /* chroma interpolation */
    for (coord_t i = 0; i < 319; i++) {
        /* Cb */
        cb0 = dst_scanline[12 * i + 0];
        cb1 = dst_scanline[12 * i + 12];

        dst_scanline[12*i + 4] = (2 * cb0 + cb1) / 3;
        dst_scanline[12*i + 8] = (2 * cb1 + cb0) / 3;

        /* Cr */
        cr0 = dst_scanline[12 * i + 2];
        cr1 = dst_scanline[12 * i + 14];

        dst_scanline[12*i +  6] = (2 * cr0 + cr1) / 3;
        dst_scanline[12*i + 10] = (2 * cr1 + cr0) / 3;
    }
}

/* interpolate a scanline between two other scanlines */
void interpolate_scanline(uint8_t *dst, uint8_t *s1, uint8_t *s2, int interp) {
    uint16_t i1, i2;
    for (coord_t i = 0; i < 2*1920; i++) {
        i1 = s1[i];
        i2 = s2[i];
        dst[i] = (i1 * (3 - interp) + i2 * interp) / 3;
    }
}

/* upscale a 480i frame to 1080i in some sort of way */
void do_upscale(RawFrame *out, uint8_t *in) {
    coord_t offset = 480 / 2 - 180; /* where to start vertically */

    /* pass 1: up-scale all scanlines that are direct copies */
    for (coord_t i = 0; i < 180; i++) {
        /* keep interlaced pairs together */
        uint8_t *even = in + 1280 * (2*i + offset);
        uint8_t *odd = even + 1280; 
        upscale_scanline(out->scanline(6*i), even);
        upscale_scanline(out->scanline(6*i + 1), odd);
    }

    /* pass 2: interpolate scanlines in between */
    for (coord_t i = 0; i < 179; i++) {
        interpolate_scanline(out->scanline(6*i+2), /* dst. scanline */
            out->scanline(6*i), out->scanline(6*i+6), /* source scanlines */
            1 /* interpolant */
        );
        interpolate_scanline(out->scanline(6*i+3), /* dst. scanline */
            out->scanline(6*i+1), out->scanline(6*i+7), /* source scanlines */
            1 /* interpolant */
        );

        interpolate_scanline(out->scanline(6*i+4), /* dst. scanline */
            out->scanline(6*i), out->scanline(6*i+6), /* source scanlines */
            2 /* interpolant */
        );
        interpolate_scanline(out->scanline(6*i+5), /* dst. scanline */
            out->scanline(6*i+1), out->scanline(6*i+7), /* source scanlines */
            2 /* interpolant */
        );
            
    }
}

class V4L2UpscaledInputAdapter : public InputAdapter,
        public Thread {
    public:
        V4L2UpscaledInputAdapter(const char *device = "/dev/video0");
        ~V4L2UpscaledInputAdapter( );

        virtual Pipe<RawFrame *> &output_pipe( ) { return out_pipe; }
        virtual Pipe<AudioPacket *> *audio_output_pipe( ) { return NULL; }

    private:
        V4L2UpscaledInputAdapter(const V4L2UpscaledInputAdapter &cam); 

        void xioctl(int ioc, void *data);
        void check_mjpeg_support( );
        void dump_image_format( );
        void set_image_format( );
        void map_frame_buffers( );
        void queue_buffer(int bufno);
        int dequeue_buffer(size_t &size);
        void queue_all_buffers( );
        void cleanup_frame_buffers( );
        void stream_on( );
        void stream_off( );
        void run_thread( );

        int fd;

        Pipe<RawFrame *> out_pipe;

        struct frame_buffer {
            void *start;
            size_t length;
        } *buffers;

        int n_buffers;

};

/* open Video4Linux2 device and try to set up certain parameters */
V4L2UpscaledInputAdapter::V4L2UpscaledInputAdapter(const char *device) 
        : out_pipe(32) {
    buffers = NULL;
    n_buffers = 0;

    fd = open(device, O_RDWR);
    if (fd == -1) {
        throw POSIXError("open v4l device");
    }

    set_image_format( );
    map_frame_buffers( );
    queue_all_buffers( );
    stream_on( );
    start_thread( );    
}

V4L2UpscaledInputAdapter::V4L2UpscaledInputAdapter(
        const V4L2UpscaledInputAdapter &cam
) : out_pipe(32) {
    (void) cam;
    assert(false);
}

V4L2UpscaledInputAdapter::~V4L2UpscaledInputAdapter( ) {
    stream_off( );
    cleanup_frame_buffers( );
    close(fd);
}

void V4L2UpscaledInputAdapter::xioctl(int ioc, void *data) {
    if (ioctl(fd, ioc, data) == -1) {
        throw POSIXError("ioctl");
    }
}

void V4L2UpscaledInputAdapter::dump_image_format( ) {
    struct v4l2_format format;

    memset(&format, 0, sizeof(format));

    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    xioctl(VIDIOC_G_FMT, &format);

    fprintf(stderr, "current:\n");
    fprintf(stderr, "    width: %d\n", format.fmt.pix.width);
    fprintf(stderr, "    height: %d\n", format.fmt.pix.height);
    fprintf(stderr, "    pixelformat: %d\n", format.fmt.pix.pixelformat);
    fprintf(stderr, "    field: %d\n", format.fmt.pix.field);
    fprintf(stderr, "    bytesperline: %d\n", format.fmt.pix.bytesperline);
}

void V4L2UpscaledInputAdapter::set_image_format( ) {
    struct v4l2_format format;
    
    /* make sure we are in NTSC mode */
    v4l2_std_id standard = V4L2_STD_NTSC_M;
    xioctl(VIDIOC_S_STD, &standard);

    /* set up desired image size */
    memset(&format, 0, sizeof(format));

    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    xioctl(VIDIOC_G_FMT, &format);

    format.fmt.pix.pixelformat = V4L2_PIX_FMT_UYVY;
    format.fmt.pix.width = 640;
    format.fmt.pix.height = 480;

    xioctl(VIDIOC_S_FMT, &format);
    dump_image_format( );
}

void V4L2UpscaledInputAdapter::map_frame_buffers( ) {
    struct v4l2_requestbuffers reqbuf;
    struct v4l2_buffer buf;
    unsigned int i;

    memset(&reqbuf, 0, sizeof(reqbuf));
    reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbuf.memory = V4L2_MEMORY_MMAP;
    reqbuf.count = 8;

    xioctl(VIDIOC_REQBUFS, &reqbuf);

    buffers = new frame_buffer[reqbuf.count];
    n_buffers = reqbuf.count;

    for (i = 0; i < reqbuf.count; i++) {
        memset(&buf, 0, sizeof(buf));
        buf.type = reqbuf.type;
        buf.memory = reqbuf.memory;
        buf.index = i;

        xioctl(VIDIOC_QUERYBUF, &buf);
        buffers[i].length = buf.length;
        buffers[i].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE,
                MAP_SHARED, fd, buf.m.offset);
    
        if (MAP_FAILED == buffers[i].start) {
            perror("mmap");
            exit(1);
        }
    }
}

void V4L2UpscaledInputAdapter::queue_buffer(int bufno) {
    struct v4l2_buffer buf;

    memset(&buf, 0, sizeof(buf));

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = bufno;

    xioctl(VIDIOC_QBUF, &buf);
}

int V4L2UpscaledInputAdapter::dequeue_buffer(size_t &bufsize) {
    struct v4l2_buffer buf;

    memset(&buf, 0, sizeof(buf));

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    xioctl(VIDIOC_DQBUF, &buf);

    bufsize = buf.bytesused;
    return buf.index;
}

void V4L2UpscaledInputAdapter::queue_all_buffers( ) {
    int i;
    for (i = 0; i < n_buffers; i++) {
        queue_buffer(i);
    }
}

void V4L2UpscaledInputAdapter::cleanup_frame_buffers(void) {
    int i;
    for (i = 0; i < n_buffers; i++) {
        munmap(buffers[i].start, buffers[i].length);
    }

    delete [] buffers;
    buffers = NULL;
    n_buffers = 0;
}

void V4L2UpscaledInputAdapter::stream_on( ) {
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    xioctl(VIDIOC_STREAMON, &type);
}

void V4L2UpscaledInputAdapter::stream_off( ) {
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    xioctl(VIDIOC_STREAMOFF, &type);
}

void V4L2UpscaledInputAdapter::run_thread( ) {
    int bufno;
    size_t bufsize;
    RawFrame *out;

    for (;;) {
        bufno = dequeue_buffer(bufsize);
        
        out = new RawFrame(1920, 1080, RawFrame::CbYCrY8422);
        if (out_pipe.can_put( )) {
            do_upscale(out, (uint8_t *) buffers[bufno].start); 
            out_pipe.put(out);
        } else {
            fprintf(stderr, "V4L2 in: dropping input frame on floor\n");
            delete out;
        }

        queue_buffer(bufno);

    }
}

InputAdapter *create_v4l2_upscaled_input_adapter(const char *device) {
    return new V4L2UpscaledInputAdapter(device);
}
