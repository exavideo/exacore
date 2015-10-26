/*
 * Copyright 2011, 2012, 2013 Exavideo LLC.
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

#ifndef _REPLAY_PLAYOUT_LAVF_SOURCE_H
#define _REPLAY_PLAYOUT_LAVF_SOURCE_H

#include "replay_playout_source.h"
#include "avspipe_allocators.h"
#include "audio_fifo.h"
#include <list>

/* silly ffmpeg... */
extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    /* ffmpeg headers have #define PixelFormat AVPixelFormat somewhere. 
     * That's bad. */
    #undef PixelFormat
}

class ReplayPlayoutLavfSource : public ReplayPlayoutSource {
    public:
        ReplayPlayoutLavfSource(const char *filename);
        ~ReplayPlayoutLavfSource( );

        void read_frame(ReplayPlayoutFrame &frame_data, Rational speed);
        static timecode_t get_file_duration(const char *filename);
        timecode_t position( );
        timecode_t duration( );
    private:
        /* call first, makes sure av_register_all has been called */
        static void ensure_registered( );

        /* try to put more stuff in the queues */
        int run_lavc( );

        AVFormatContext *format_ctx;
        int video_stream;
        int audio_stream;

        AVCodecContext *video_codecctx;
        AVCodec *video_codec;
        AVFrame *lavc_frame;

        AVCodecContext *audio_codecctx;
        AVCodec *audio_codec;
        AVFrame *audio_frame;

        timecode_t n_frames;
        AudioFIFO<int16_t> pending_audio;

        /* uninitialized / static data */
        AvspipeNTSCSyncAudioAllocator audio_allocator;
        static int registered;

        std::list<RawFrame *> pending_video_frames;
};

#endif
