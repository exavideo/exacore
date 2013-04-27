/*
 * Copyright 2011 Andrew H. Armenia.
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

#ifndef _OPENREPLAY_DECKLINK_H
#define _OPENREPLAY_DECKLINK_H

#include "adapter.h"

OutputAdapter *create_decklink_output_adapter(unsigned int card_index,
        unsigned int decklink_norm, RawFrame::PixelFormat pf);

OutputAdapter *create_decklink_output_adapter_with_audio(
        unsigned int card_index, unsigned int decklink_norm, 
        RawFrame::PixelFormat pf, unsigned int n_channels = 2);

InputAdapter *create_decklink_input_adapter(unsigned int card_index,
        unsigned int decklink_norm, unsigned int decklink_input,
        RawFrame::PixelFormat pf);

InputAdapter *create_decklink_input_adapter_with_audio(unsigned int card_index,
        unsigned int decklink_norm, unsigned int decklink_input,
        RawFrame::PixelFormat pf, unsigned int n_channels = 2);

InputAdapter *create_decklink_audio_input_adapter(
    unsigned int card_index,
    unsigned int decklink_norm,
    unsigned int decklink_input,
    unsigned int n_channels = 2
);

#endif
