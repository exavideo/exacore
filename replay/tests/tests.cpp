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

#include "buffer.h"
#define BOOST_TEST_MODULE replay
#define BOOST_AUTO_TEST_MAIN
#include <boost/test/unit_test.hpp>
#include <boost/test/auto_unit_test.hpp>
#include <boost/test/included/unit_test_framework.hpp>

BOOST_AUTO_TEST_CASE(rel_offset) {
    /* FIXME mmap /dev/zero may not be portable */
    Buffer b("/dev/zero", 4096, 16384);

    BOOST_CHECK( b.rel_offset(0, 0) == 0 );
    BOOST_CHECK( b.rel_offset(0, 1) == 1 );
    BOOST_CHECK( b.rel_offset(0, -1) == 4095 );
    BOOST_CHECK( b.rel_offset(0, 4096) == 0 );
    BOOST_CHECK( b.rel_offset(0, -4096) == 0 );
    BOOST_CHECK( b.rel_offset(0, 4100) == 4 );
    BOOST_CHECK( b.rel_offset(0, -4100) == 4092 );
    BOOST_CHECK( b.rel_offset(0, 8192) == 0 );
    BOOST_CHECK( b.rel_offset(0, -8192) == 0 );

    BOOST_CHECK( b.rel_offset(2000, 1) == 2001 );
    BOOST_CHECK( b.rel_offset(2000, -1) == 1999 );
    BOOST_CHECK( b.rel_offset(2000, 4096) == 2000 );
    BOOST_CHECK( b.rel_offset(2000, -4096) == 2000 );
    BOOST_CHECK( b.rel_offset(2000, 4100) == 2004 );
    BOOST_CHECK( b.rel_offset(2000, -4100) == 1996 );
    BOOST_CHECK( b.rel_offset(2000, 8192) == 2000 );
    BOOST_CHECK( b.rel_offset(2000, -8192) == 2000 );

    BOOST_CHECK( b.rel_offset(4095, 1) == 0 );
    BOOST_CHECK( b.rel_offset(4095, -1) == 4094 );
    BOOST_CHECK( b.rel_offset(4095, 4096) == 4095 );
    BOOST_CHECK( b.rel_offset(4095, -4096) == 4095 );
    BOOST_CHECK( b.rel_offset(4095, 4100) == 3 );
    BOOST_CHECK( b.rel_offset(4095, -4100) == 4091 );
    BOOST_CHECK( b.rel_offset(4095, 8192) == 4095 );
    BOOST_CHECK( b.rel_offset(4095, -8192) == 4095 );
}

BOOST_AUTO_TEST_CASE(in_range) {
    Buffer b("/dev/zero", 4096, 16384);

    BOOST_CHECK( b.in_range(0, 5, 3) );
    BOOST_CHECK( !b.in_range(0, 5, 8) );
    BOOST_CHECK( b.in_range(5, 0, 6) );
    BOOST_CHECK( !b.in_range(5, 0, 2) );
    BOOST_CHECK( b.in_range(0, 0, 0) );

}

BOOST_AUTO_TEST_CASE(span) {
    Buffer b("/dev/zero", 4096, 16384);

    BOOST_CHECK( b.span(0, 0) == 1 );
    BOOST_CHECK( b.span(200, 200) == 1 );
    BOOST_CHECK( b.span(4095, 4095) == 1 );

    BOOST_CHECK( b.span(200, 400) == 201 );
    BOOST_CHECK( b.span(400, 200) == 4096 - 199 );
}

BOOST_AUTO_TEST_CASE(intersect) {
    Buffer b("/dev/zero", 4096, 16384);
    
    /* enumerated all possibilities of the order of a1, a2, b1, b2 */
    BOOST_CHECK( b.intersect(4, 8, 12, 16) == 0 );
    BOOST_CHECK( b.intersect(4, 4, 12, 16) == 0 );
    BOOST_CHECK( b.intersect(4, 8, 12, 12) == 0 );
    BOOST_CHECK( b.intersect(4, 4, 12, 12) == 0 );
    
    BOOST_CHECK( b.intersect(4, 8, 16, 12) == 5 );
    BOOST_CHECK( b.intersect(4, 4, 16, 12) == 1 );

    BOOST_CHECK( b.intersect(4, 12, 8, 16) == 5 );
    BOOST_CHECK( b.intersect(4, 4, 8, 16) == 0 );
    BOOST_CHECK( b.intersect(4, 12, 8, 8) == 1 );

    BOOST_CHECK( b.intersect(4, 16, 8, 12) == 5 );

    BOOST_CHECK( b.intersect(4, 12, 8, 16) == 5 );

    BOOST_CHECK( b.intersect(4, 16, 12, 8) == 10 );

    BOOST_CHECK( b.intersect(8, 4, 12, 16) == 5 );

    BOOST_CHECK( b.intersect(8, 4, 16, 12) == 4090 );

    BOOST_CHECK( b.intersect(12, 4, 8, 16) == 5 );

    BOOST_CHECK( b.intersect(16, 4, 8, 12) == 0 );

    BOOST_CHECK( b.intersect(12, 4, 16, 8) == 4085 );

    BOOST_CHECK( b.intersect(16, 4, 12, 8) == 4085 );

    BOOST_CHECK( b.intersect(8, 12, 4, 16) == 5 );

    BOOST_CHECK( b.intersect(8, 16, 4, 12) == 5 );

    BOOST_CHECK( b.intersect(12, 8, 4, 16) == 10 );

    BOOST_CHECK( b.intersect(16, 8, 4, 12) == 5 );

    BOOST_CHECK( b.intersect(12, 16, 4, 8) == 0 );

    BOOST_CHECK( b.intersect(16, 12, 4, 8) == 5 );

    BOOST_CHECK( b.intersect(8, 12, 16, 4) == 0 );

    BOOST_CHECK( b.intersect(8, 16, 12, 4) == 5 );

    BOOST_CHECK( b.intersect(12, 8, 16, 4) == 4085 );

    BOOST_CHECK( b.intersect(16, 8, 12, 4) == 4085 );

    BOOST_CHECK( b.intersect(12, 16, 8, 4) == 5 );

    BOOST_CHECK( b.intersect(16, 12, 8, 4) == 4090 );

}

