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

#include "replay_playout_image_filter.h"
#include "rsvg_frame.h"

ReplayPlayoutImageFilter::ReplayPlayoutImageFilter(
        RawFrame *image_,
        coord_t x_,
        coord_t y_
) {
    image = image_;
    x = x_;
    y = y_;
    enabled = true;
}

ReplayPlayoutImageFilter::~ReplayPlayoutImageFilter( ) {
    delete image;
}

void ReplayPlayoutImageFilter::enable( ) {
    enabled = true;
}

void ReplayPlayoutImageFilter::disable( ) {
    enabled = false;
}

bool ReplayPlayoutImageFilter::is_enabled( ) {
    return enabled;
}

void ReplayPlayoutImageFilter::process_frame(ReplayPlayoutFrame &frame) {
    if (enabled) {
        frame.video_data->draw->alpha_key(x, y, image, 255);  
    }
}

ReplayPlayoutImageFilter *ReplayPlayoutImageFilter::from_svg(
        const char *path,
        coord_t x,
        coord_t y
) {
    RawFrame *image = RsvgFrame::render_svg_file(path);
    return new ReplayPlayoutImageFilter(image, x, y);
}

ReplayPlayoutImageFilter *ReplayPlayoutImageFilter::from_png(
        const char *path,
        coord_t x,
        coord_t y
) {
    RawFrame *image = RawFrame::from_image_file(path);
    return new ReplayPlayoutImageFilter(image, x, y);
}
