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

%include "raw_frame.i"

%{
    #include "decklink.h"
%}

/* FIXME these will leak. For now this is sort of by design. */

OutputAdapter *create_decklink_output_adapter(unsigned int, 
        unsigned int, RawFrame::PixelFormat);

OutputAdapter *create_decklink_output_adapter_with_audio(unsigned int, 
        unsigned int, RawFrame::PixelFormat);

InputAdapter *create_decklink_input_adapter(unsigned int, unsigned int,
        unsigned int, RawFrame::PixelFormat);

InputAdapter *create_decklink_input_adapter_with_audio(unsigned int, 
        unsigned int, unsigned int, RawFrame::PixelFormat);

