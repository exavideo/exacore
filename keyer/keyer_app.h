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

#include <vector>
#include "adapter.h"
#include "character_generator.h"

class KeyerApp {
    protected:
        struct CgEntry {
            CharacterGenerator *cg;
            uint64_t saved_tally_bits;
            bool tie_inhibit;
            bool was_tied;

            CgEntry(CharacterGenerator *cg_) {
                cg = cg_;
                saved_tally_bits = 0;
                was_tied = false;
                tie_inhibit = false;
            }
        };
        std::vector<CgEntry> cgs;
        InputAdapter *iadp;
        std::vector<OutputAdapter *> oadps;
        std::vector<bool> flags;

        void clear_all_flags( );
    public:
        KeyerApp( );
        ~KeyerApp( );

        void input(InputAdapter *iadp_);
        void output(OutputAdapter *oadp_);
        void cg(CharacterGenerator *cg);
        void run( );
};
