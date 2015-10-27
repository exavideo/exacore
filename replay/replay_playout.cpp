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

#include "replay_playout.h"
#include "replay_playout_buffer_source.h"
#include "replay_playout_avspipe_source.h"
#include "replay_playout_lavf_source.h"
#include "replay_playout_queue_source.h"

ReplayPlayout::ReplayPlayout(OutputAdapter *oadp_) {
    oadp = oadp_;
    idle_source = new ReplayPlayoutBarsSource;
    playout_source = NULL;
    new_speed = NULL;
    
    _source_position = 0;
    _source_duration = -1;

    start_thread( );
}

ReplayPlayout::~ReplayPlayout( ) {
    delete idle_source;
}

void ReplayPlayout::run_thread( ) {
    ReplayPlayoutSource *active_source = idle_source; 
    ReplayPlayoutSource *next_source;
    ReplayPlayoutFrame frame_data;
    ReplayRawFrame *monitor_frame;
    Rational current_speed(1,1);
    Rational *next_speed;

    priority(SCHED_RR, 40);

    for (;;) {
        /* is there a next source available? if so, we take it */
        next_source = playout_source.exchange(NULL);
        if (next_source != NULL) {
            if (next_source == idle_source) {
                /* 
                 * we've been commanded to stop playout
                 *
                 * try to read another frame from the current source 
                 * if we get one, use that as the idle frame
                 */
                active_source->read_frame(frame_data, current_speed);
                if (frame_data.video_data != NULL) {
                    idle_source->set_frame(frame_data.video_data);
                }
            }

            if (active_source != idle_source) {
                delete active_source;
            }
            active_source = next_source;
            active_source->set_output_dominance(oadp->output_dominance( ));
        }

        /* atomically acquire speed commands */
        next_speed = new_speed.exchange(NULL);
        if (next_speed != NULL) {
            current_speed = *next_speed;
            delete next_speed;
        }

        /* read data from currently active source */
        active_source->read_frame(frame_data, current_speed);

        /* 
         * if we got video, output it. If not,
         * switch to idle source and try again 
         */
        if (frame_data.video_data != NULL) {
            /* apply filters to frame */
            { MutexLock l(filters_mutex);
                for (unsigned i = 0; i < filters.size( ); i++) {
                    filters[i]->process_frame(frame_data);
                }
            }

            /* create monitor frame */
            monitor_frame = new ReplayRawFrame(
                frame_data.video_data->convert->BGRAn8_scale_1_2( )
            );
            monitor_frame->source_name = "Program";
            monitor_frame->source_name2 = frame_data.source_name;
            monitor_frame->tc = frame_data.tc;
            monitor_frame->fractional_tc = frame_data.fractional_tc;
            monitor.put(monitor_frame);

            /* write data to output */
            oadp->input_pipe( ).put(frame_data.video_data);
            if (oadp->audio_input_pipe( )) {
                oadp->audio_input_pipe( )->put(frame_data.audio_data);
            } else {
                delete frame_data.audio_data;
            }
        } else {        
            if (active_source != idle_source) {
                delete active_source;
            }
            active_source = idle_source;
        }

        /* Update source state */
        _source_position = active_source->position( );
        _source_duration = active_source->duration( );
    }
}

void ReplayPlayout::set_source(ReplayPlayoutSource *src) {
    ReplayPlayoutSource *old_src = playout_source.exchange(src);
    if (old_src != NULL) {
        delete old_src;
    }
}

/* compatibility wrapper */
void ReplayPlayout::roll_shot(const ReplayShot &shot) {
    /* copy channel mappings to playout source */
    ReplayPlayoutBufferSource *newsrc = new ReplayPlayoutBufferSource(shot);
    for (ChannelMapEntry &ch : channel_map) {
        newsrc->map_channel(ch.no, ch.buf);
    }

    set_source(newsrc);
}

void ReplayPlayout::map_channel(unsigned int ch, ReplayBuffer *buf) {
    ChannelMapEntry newent;
    newent.no = ch;
    newent.buf = buf;
    /* FIXME should first search the map for a pre-existing mapping */
    channel_map.push_back(newent);
}

void ReplayPlayout::clear_channel_map( ) {
    channel_map.clear( );
}

void ReplayPlayout::set_speed(int num, int denom) {
    Rational *old = new_speed.exchange(new Rational(num, denom));
    if (old != NULL) {
        delete old;
    }
}

void ReplayPlayout::stop( ) {
    set_source(idle_source);
}

void ReplayPlayout::register_filter(ReplayPlayoutFilter *filt) {
    MutexLock l(filters_mutex);
    filters.push_back(filt);
}

void ReplayPlayout::avspipe_playout(const char *cmd) {
    set_source(new ReplayPlayoutAvspipeSource(cmd));
}

void ReplayPlayout::lavf_playout(const char *file) {
    set_source(new ReplayPlayoutLavfSource(file));
}

void ReplayPlayout::lavf_playout_list(const StringList &files) {
    ReplayPlayoutQueueSource::SourceQueue sources;
    for (auto i = files.begin(); i != files.end(); i++) {
        sources.push_back(new ReplayPlayoutLavfSource(*i));
    }

    set_source(new ReplayPlayoutQueueSource(sources));
}

timecode_t ReplayPlayout::source_position( ) {
    return _source_position;
}

timecode_t ReplayPlayout::source_duration( ) {
    return _source_duration;
}

