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
    #include "rollout_preview.h"
%}

%include "typemaps.i"

%rename("monitor") RolloutPreview::get_monitor( );

class RolloutPreview : public Thread {
    public:
        RolloutPreview( );
        ~RolloutPreview( );
        
        void load_file(const char *fn);
        void seek(int64_t delta);
        void get(std::string &OUTPUT, int64_t &OUTPUT);
        AsyncPort<ReplayRawFrame> *get_monitor( );
};

