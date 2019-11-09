/*
 * Copyright 2016 Exavideo LLC.
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

#ifndef _DEBUG_FPRINTF_H
#define _DEBUG_FPRINTF_H

#ifdef VERBOSE_DEBUG
#define debug_fprintf(...) fprintf(__VA_ARGS__)
#else
#define debug_fprintf(...)
#endif

#endif
