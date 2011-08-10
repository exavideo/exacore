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

#ifndef _REPLAY_FRAME_EXTRACTOR_H
#define _REPLAY_FRAME_EXTRACTOR_H

#include "replay_data.h"
#include "mjpeg_codec.h"
#include <string>

/* I am NOT thread safe at all! Be careful! */
class ReplayFrameExtractor {
    public:
        ReplayFrameExtractor( );
        ~ReplayFrameExtractor( );

        void extract_scaled_jpeg(const ReplayShot &shot, timecode_t offset, 
                std::string &jpeg, int scale_down = 1);

        void extract_raw_jpeg(const ReplayShot &shot, timecode_t offset,
                std::string &jpeg);
    protected:
        Mjpeg422Decoder dec;
        Mjpeg422Encoder enc;
};

#endif
