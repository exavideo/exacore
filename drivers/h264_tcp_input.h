/*
 * Copyright 2015 Exavideo LLC.
 * 
 * This file is part of exacore.
 * 
 * exacore is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * exacore is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with exacore.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _H264_TCP_INPUT_H
#define _H264_TCP_INPUT_H

#include "adapter.h"
InputAdapter *create_h264_tcp_input_adapter(const char *host, const char *port);

#endif
