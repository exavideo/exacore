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

%{
    #include "replay_preview.h"
%}

%rename("shot") ReplayPreview::get_shot(ReplayShot&);
%rename("shot=") ReplayPreview::change_shot(const ReplayShot&);
%rename("monitor") ReplayPreview::get_monitor( );

class ReplayPreview : public Thread {
    public:
        ReplayPreview( );
        ~ReplayPreview( );

        void change_shot(const ReplayShot &INPUT);
        void get_shot(ReplayShot &OUTPUT);

        void seek(timecode_t delta);
        void mark_in( );
        void mark_out( );
        AsyncPort<ReplayRawFrame> *get_monitor( );
};

