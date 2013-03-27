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

#include "cpu_dispatch.h"
#include <stdio.h>

#ifdef X86_64
#define cpuid(f,c,d) \
    __asm__ __volatile__("push %%rbx; push %%rax; mov %2, %%eax; cpuid; mov %%ecx, %0; mov %%edx, %1; pop %%rax; pop %%rbx":\
        "=r" (c), "=r" (d) : "r" (f) : "%eax","%ecx","%edx");
#else
#define cpuid(f,c,d) \
    __asm__ __volatile__("pushl %%ebx; pushl %%eax; mov %2, %%eax; cpuid; mov %%ecx, %0; mov %%edx, %1; popl %%eax; popl %%ebx":\
        "=r" (c), "=r" (d) : "r" (f) : "%eax","%ecx","%edx");
#endif

static bool force_no_simd = false;
static bool cpuid_done = false;
static int cpuid_ecx, cpuid_edx;

void cpu_force_no_simd( ) {
    force_no_simd = true;
}

static void print_cpu_info( ) {
    fprintf(stderr, "CPU dispatch initialized\n");
    fprintf(stderr, "cx=0x%x dx=0x%x ", cpuid_ecx, cpuid_edx);
    if (cpu_sse2_available( )) {
        fprintf(stderr, "SSE2 ");
    }
    if (cpu_sse3_available( )) {
        fprintf(stderr, "SSE3 ");
    }
    if (cpu_ssse3_available( )) {
        fprintf(stderr, "SSSE3 ");
    }
    if (cpu_sse41_available( )) {
        fprintf(stderr, "SSE41 ");
    }
    fprintf(stderr, "\n");
}

static void cpuid_init( ) {
    if (!cpuid_done) {
        cpuid(0x1, cpuid_ecx, cpuid_edx);
        cpuid_done = true;

        print_cpu_info( );
    }
}

bool cpu_sse2_available( ) {
    cpuid_init( );

    if (force_no_simd) {
        return false;
    } else if (cpuid_edx & 0x04000000) {
        return true;
    } else {
        return false;
    }
}

bool cpu_sse3_available( ) {
    cpuid_init( );
    
    if (force_no_simd) {
        return false;
    } else if (cpuid_ecx & 0x00000001) {
        return true;
    } else {
        return false;
    }
}

bool cpu_ssse3_available( ) {
    cpuid_init( );
    
    if (force_no_simd) {
        return false;
    } else if (cpuid_ecx & 0x00000200) {
        return true;
    } else {
        return false;
    }
}

bool cpu_sse41_available( ) {
    cpuid_init( );
    if (force_no_simd) {
        return false;
    } else if (cpuid_ecx & 0x00080000) {
        return true;
    } else {
        return false;
    }
}
