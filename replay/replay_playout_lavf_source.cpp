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

/* based on github.com/chelyaev/ffmpeg-tutorial */

#include "replay_playout_lavf_source.h"

timecode_t ReplayPlayoutLavfSource::get_file_duration(
        const char *filename 
) {
    timecode_t n_frames = 0;

    AVFormatContext *format_ctx = NULL;
    ensure_registered( );

    if (avformat_open_input(&format_ctx, filename, NULL, NULL) != 0) {
        throw std::runtime_error("avformat_open_input failed");
    }

    if (avformat_find_stream_info(format_ctx, NULL) < 0) {
        throw std::runtime_error("avformat_find_stream_info failed");
    }

    for (unsigned int i = 0; i < format_ctx->nb_streams; i++) {
        if (format_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            n_frames = format_ctx->streams[i]->nb_frames;
        }
    }
    
    avformat_close_input(&format_ctx);
    return n_frames;
}

ReplayPlayoutLavfSource::ReplayPlayoutLavfSource(const char *filename) : 
    format_ctx(NULL),
    video_stream(-1),
    audio_stream(-1),
    video_codecctx(NULL),
    video_codec(NULL),
    lavc_frame(NULL),
    audio_codecctx(NULL),
    audio_codec(NULL),
    n_frames(0),
    pending_audio(2 /* stereo */)
{
	ensure_registered( );
	
    audio_frame = av_frame_alloc();
    if (!audio_frame) {
        throw std::runtime_error("av_frame_alloc failed");
    }

    // Try to open file
    if (avformat_open_input(&format_ctx, filename, NULL, NULL) != 0) {
        throw std::runtime_error("avformat_open_input failed");
    }

    if (avformat_find_stream_info(format_ctx, NULL) < 0) {
        throw std::runtime_error("avformat_find_stream_info failed");
    }

    av_dump_format(format_ctx, 0, filename, 0);

    // find video stream
    video_stream = -1;
    audio_stream = -1;
    for (unsigned int i = 0; i < format_ctx->nb_streams; i++) {
        if (format_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream = i;
        }

        if (format_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream = i;
        }
    }

    if (video_stream == -1) {
        throw std::runtime_error("could not find video stream!");
    }
    if (audio_stream == -1) {
        throw std::runtime_error("could not find audio stream!");
    }

    // try to find and open video codec
    video_codecctx = format_ctx->streams[video_stream]->codec;
    video_codec = avcodec_find_decoder(video_codecctx->codec_id);
    if (video_codec == NULL) {
        throw std::runtime_error("unsupported video codec!");   
    }

    if (avcodec_open2(video_codecctx, video_codec, NULL) < 0) {
        throw std::runtime_error("failed to open codec!");
    }

    lavc_frame = av_frame_alloc();    

    // try to find and open audio codec
    audio_codecctx = format_ctx->streams[audio_stream]->codec;
    audio_codec = avcodec_find_decoder(audio_codecctx->codec_id);
    if (audio_codec == NULL) {
        throw std::runtime_error("unsupported audio codec!");
    }

    if (avcodec_open2(audio_codecctx, audio_codec, NULL) < 0) {
        throw std::runtime_error("failed to open audio codec");
    }

}

ReplayPlayoutLavfSource::~ReplayPlayoutLavfSource( ) {
    av_free(lavc_frame);
    avcodec_close(video_codecctx);
    avformat_close_input(&format_ctx);
}

void ReplayPlayoutLavfSource::ensure_registered( ) {
    if (!registered) {
        registered = 1;
        av_register_all( );
    }
}

void ReplayPlayoutLavfSource::seek(int64_t usec) {
	int ret;

	ret = av_seek_frame(format_ctx, -1, usec, 0);

	if (ret < 0) {
		fprintf(stderr, "av_seek_frame() failed!\n");
	}
}

void ReplayPlayoutLavfSource::read_frame(
        ReplayPlayoutFrame &frame_data, 
        Rational speed
) {

    frame_data.tc = 0;
    frame_data.fractional_tc = Rational(0);
    frame_data.source_name = "LAVF Rollout";
    frame_data.video_data = NULL;
    frame_data.audio_data = NULL;

    IOAudioPacket *audio = audio_allocator.allocate( );

    (void) speed;

    while (pending_video_frames.size( ) == 0 
            || pending_audio.fill_samples( ) < audio->size_samples( )) {
        if (run_lavc( ) == 0) {
            delete audio;
            return;
        }
    }

    frame_data.video_data = pending_video_frames.front( );
    pending_video_frames.pop_front( );
    
    /* something something dequeue sample frames... */
    pending_audio.fill_packet(audio);
    frame_data.audio_data = audio;

    n_frames++;
}

static void copy_fltp(
    AVFrame *audio_frame, 
    PackedAudioPacket<int16_t> &apkt
) {
    int16_t *d;
    float *s;
    int ch = apkt.channels( );
    int ns = apkt.size_samples( );

    for (int i = 0; i < ch; i++) {
        d = apkt.data( ) + i;
        s = (float *)(audio_frame->data[i]);
        for (int j = 0; j < ns; j++) {
            *d = (int16_t)(*s * 32767.0);
            d += ch;
            s ++;
        }
    }
}

int ReplayPlayoutLavfSource::run_lavc( ) {
    AVPacket packet;
    int frame_finished = 0;
    int audio_finished = 0;

    /* 
     * read stream until we get a video frame, 
     * possibly also decoding some audio along the way
     */
    while (frame_finished == 0 && audio_finished == 0 &&
            av_read_frame(format_ctx, &packet) >= 0) {
        if (packet.stream_index == video_stream) {
            avcodec_decode_video2(video_codecctx, lavc_frame, 
                    &frame_finished, &packet);
        } else if (packet.stream_index == audio_stream) {
            avcodec_decode_audio4(audio_codecctx, audio_frame, 
                    &audio_finished, &packet);
        }

        av_free_packet(&packet);
    }

    if (frame_finished) {
        /* make a RawFrame out of lavc_frame */
        RawFrame *fr = new RawFrame(1920, 1080, RawFrame::CbYCrY8422);
        switch (lavc_frame->format) {
            case AV_PIX_FMT_YUVJ422P:
            case AV_PIX_FMT_YUV422P:
                fr->pack->YCbCr8P422(
                    lavc_frame->data[0], 
                    lavc_frame->data[1],
                    lavc_frame->data[2],
                    lavc_frame->linesize[0],
                    lavc_frame->linesize[1],
                    lavc_frame->linesize[2]
                );
                break;

            case AV_PIX_FMT_UYVY422:
                /* copy stuff */
                memcpy(fr->data( ), lavc_frame->data[0], fr->size( ));
                break;

            case AV_PIX_FMT_YUV420P:
                fr->pack->YCbCr8P420(
                    lavc_frame->data[0],
                    lavc_frame->data[1],
                    lavc_frame->data[2],
                    lavc_frame->linesize[0],
                    lavc_frame->linesize[1],
                    lavc_frame->linesize[2]
                );
                break;

            default:
                fprintf(stderr, "ReplayPlayoutLavfSource doesn't know how "
                    "to handle AVPixelFormat %d\n", lavc_frame->format);
                memset(fr->data( ), 128, fr->size( ));
                break;
        }

        pending_video_frames.push_back(fr);
        return 1;
    } else if (audio_finished) {
        PackedAudioPacket<int16_t> apkt(
            audio_frame->nb_samples,
            audio_codecctx->channels
        );

        if (audio_codecctx->sample_fmt == AV_SAMPLE_FMT_S16) {
            memcpy(apkt.data( ), audio_frame->data[0], apkt.size_bytes( ));
        } else if (audio_codecctx->sample_fmt == AV_SAMPLE_FMT_FLTP) {
            /* convert planar float (from AAC) to signed 16-bit */
            copy_fltp(audio_frame, apkt);
        } else {
            fprintf(stderr, "sample_fmt=%d\n", audio_codecctx->sample_fmt);
            throw std::runtime_error("don't understand sample format");
        }
        if (audio_codecctx->sample_rate != 48000) {
            throw std::runtime_error("need 48khz");
        }

        if (audio_codecctx->channels != 2) {
            /* mix down to 2 channels if needed */
            PackedAudioPacket<int16_t> *twoch = apkt.change_channels(2);
            pending_audio.add_packet(twoch);
            delete twoch;
        } else {
            pending_audio.add_packet(&apkt);
        }

        return 1;
    } else {
        return 0;
    }
}

timecode_t ReplayPlayoutLavfSource::position( ) {
    return n_frames;
}

timecode_t ReplayPlayoutLavfSource::duration( ) {
    return format_ctx->streams[video_stream]->nb_frames;
}
int ReplayPlayoutLavfSource::registered = 0;
