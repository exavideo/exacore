/*
 * Copyright 2011, 2013 Exavideo LLC.
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

#ifndef _ROLLOUT_PREVIEW_H
#define _ROLLOUT_PREVIEW_H

#include "thread.h"
#include "mutex.h"
#include "condition.h"
#include "async_port.h"
#include "replay_data.h"
#include <stdexcept>
#include <string>

/* 
 * RolloutPreview objects allow interactive seeking of 
 * ReplayPlayoutLavfSources in the multiviewer
 */
class RolloutPreview : public Thread {
    public:
        RolloutPreview( );
        ~RolloutPreview( );

	void load_file(const char *fn);
        void seek(int64_t delta); 
	void get(std::string &fn, int64_t &pos);

        AsyncPort<ReplayRawFrame> monitor;
        AsyncPort<ReplayRawFrame> *get_monitor( ) { return &monitor; }

    protected:
        void run_thread( );
        void wait_update(std::string &new_filename, int64_t &new_pos);

	std::string filename;
	int64_t pos;

        Mutex m;
        Condition updated;
};

#endif

