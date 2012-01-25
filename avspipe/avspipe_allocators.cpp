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

#include "avspipe_allocators.h"

AvspipeRawFrame1080Allocator::AvspipeRawFrame1080Allocator( ) {

}

RawFrame * AvspipeRawFrame1080Allocator::allocate( ) {
    return new RawFrame(1920, 1080, RawFrame::CbYCrY8422);
}





AvspipeNTSCSyncAudioAllocator::AvspipeNTSCSyncAudioAllocator( ) {
    frame = 0;
}

AudioPacket *AvspipeNTSCSyncAudioAllocator::allocate( ) {
    AudioPacket *ret;
    
    /*
     * the goal is to generate a sequence like this:
     *  0       1       2       3       4
     *  1602    1601    1602    1601    1602
     * 
     * that makes a total of 8,008 audio sample frames
     * for every 5 NTSC frames.
     * 5 * 1,001 / 30,000 * 48,000 = 8,008 exactly,
     * so this rate is locked to the video rate.
     */

    if (frame == 1 || frame == 3) {
        ret = new AudioPacket(48000, 2, 2, 1601);
    } else {
        ret = new AudioPacket(48000, 2, 2, 1602);
    }

    frame++;
    if (frame >= 5) {
        frame = 0;
    }

    return ret;
}
