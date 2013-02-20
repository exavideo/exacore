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

#ifndef _REPLAY_PLAYOUT_IMAGE_FILTER_H
#define _REPLAY_PLAYOUT_IMAGE_FILTER_H

#include "replay_playout_filter.h"
#include "raw_frame.h"
#include <atomic>

class ReplayPlayoutImageFilter : public ReplayPlayoutFilter {
    public:
        /* Takes ownership of image_ */
        ReplayPlayoutImageFilter(RawFrame *image_, coord_t x_, coord_t y_);
        ~ReplayPlayoutImageFilter( );

        virtual void enable( );
        virtual void disable( );
        virtual bool is_enabled( );
        virtual void process_frame(ReplayPlayoutFrame &frame);

        /* Create filter from png file */
        static ReplayPlayoutImageFilter *from_png(
            const char *path, coord_t x, coord_t y
        );

        /* Create from svg file */
        static ReplayPlayoutImageFilter *from_svg(
            const char *path, coord_t x, coord_t y
        );
    protected:
        std::atomic<bool> enabled;
        RawFrame *image;
        coord_t x, y;
};

#endif
