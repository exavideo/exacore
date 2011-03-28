; Copyright 2011 Andrew H. Armenia.
; 
; This file is part of openreplay.
; 
; openreplay is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation, either version 3 of the License, or
; (at your option) any later version.
; 
; openreplay is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
; 
; You should have received a copy of the GNU General Public License
; along with openreplay.  If not, see <http://www.gnu.org/licenses/>.

bits 64

; As of 3-14-11, this code is untested because I don't have a SSSE3 processor.

section text align=16
global CbYCrY8422_YCbCr8P422_ssse3

CbYCrY8422_YCbCr8P422_ssse3:
    ; argument summary...
    ; rdi = packed data size
    ; rsi = packed data pointer
    ; rdx = Y plane pointer
    ; rcx = Cb plane pointer
    ; r8 = Cr plane pointer

    movdqa xmm0, [rsi]          ; rsi = [uyvyuyvyuyvyuyvy]
    add rsi, 16
   
    movdqa xmm1, xmm0
    movdqa xmm2, xmm0

    pshufb xmm0, [Y_shuffle wrt rip] ; xmm0 = [yyyyyyyy        ]
    pshufb xmm1, [U_shuffle wrt rip] ; xmm1 = [uuuu            ]
    pshufb xmm2, [V_shuffle wrt rip] ; xmm2 = [vvvv            ]

    movq [rdx], xmm0
    movd [rcx], xmm1
    movd [r8], xmm2

    add rdx, 8
    add rcx, 4
    add r8, 4

    sub rdi, 16
    jg CbYCrY8422_YCbCr8P422_ssse3

    ret

align 16

Y_shuffle:
    db 0x01, 0x03, 0x05, 0x07, 0x09, 0x0b, 0x0d, 0x0f
    db 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff

U_shuffle:
    db 0x00, 0x04, 0x08, 0x0c, 0xff, 0xff, 0xff, 0xff
    db 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff

V_shuffle:
    db 0x02, 0x06, 0x0a, 0x0e, 0xff, 0xff, 0xff, 0xff
    db 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff

; vim:syntax=nasm64
