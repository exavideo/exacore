; Copyright 2011 Exavideo LLC.
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

section text align=16
global BGRAn8_BGRAn8_composite_chunk_sse2

BGRAn8_BGRAn8_composite_chunk_sse2:
    ; rdi = background data
    ; rsi = key/fill data
    ; rdx = number of bytes to process
    ; rcx = global alpha

    ; expand global alpha into 4 32-bit copies in xmm8
    movd    xmm8, rcx
    pshufd  xmm8, xmm8, 0x00
    cvtdq2ps    xmm8, xmm8
    divps   xmm8, [two_fifty_five wrt rip]

.loop:
    movdqu  xmm0, [rsi]
    movdqa  xmm1, xmm0
    movdqa  xmm2, xmm0
    movdqa  xmm3, xmm0

    ; top alpha into xmm0
    psrld   xmm0, 24
    pand    xmm0, [lsb32_mask wrt rip]
    cvtdq2ps    xmm0, xmm0
    divps   xmm0, [two_fifty_five wrt rip]

    ; top red into xmm1
    psrld   xmm1, 16
    pand    xmm1, [lsb32_mask wrt rip]
    cvtdq2ps    xmm1, xmm1

    ; top green into xmm2
    psrld   xmm2, 8
    pand    xmm2, [lsb32_mask wrt rip]
    cvtdq2ps    xmm2, xmm2

    ; top blue into xmm3
    pand    xmm3, [lsb32_mask wrt rip]
    cvtdq2ps    xmm3, xmm3

    ; bottom alpha into xmm4
    movdqu  xmm4, [rdi]
    movdqa  xmm5, xmm4
    movdqa  xmm6, xmm4
    movdqa  xmm7, xmm4

    ; same as above to get a/r/g/b vectors
    psrld   xmm4, 24
    pand    xmm4, [lsb32_mask wrt rip]
    cvtdq2ps    xmm4, xmm4
    divps   xmm4, [two_fifty_five wrt rip]

    psrld   xmm5, 16
    pand    xmm5, [lsb32_mask wrt rip]
    cvtdq2ps    xmm5, xmm5

    psrld   xmm6, 8
    pand    xmm6, [lsb32_mask wrt rip]
    cvtdq2ps    xmm6, xmm6

    pand    xmm7, [lsb32_mask wrt rip]
    cvtdq2ps    xmm7, xmm7

    ; math time...
    ; multiply source alpha by global alpha value
    mulps   xmm0, xmm8

    ; multiply background alpha by 1-source alpha
    movdqa  xmm9, xmm0
    mulps   xmm9, xmm4
    subps   xmm4, xmm9

    ; multiply background alpha into background color
    mulps   xmm5, xmm4
    mulps   xmm6, xmm4
    mulps   xmm7, xmm4

    ; multiply key alpha into key color
    mulps   xmm1, xmm0
    mulps   xmm2, xmm0
    mulps   xmm3, xmm0

    ; add key color into background color
    addps   xmm5, xmm1
    addps   xmm6, xmm2
    addps   xmm7, xmm3

    ; compute new alpha
    addps   xmm4, xmm0

    ; divide color by new alpha
    divps   xmm5, xmm4
    divps   xmm6, xmm4
    divps   xmm7, xmm4

    ; now convert back to integers and store
    mulps	xmm4, [two_fifty_five wrt rip]
    cvtps2dq    xmm4, xmm4
    cvtps2dq    xmm5, xmm5
    cvtps2dq    xmm6, xmm6
    cvtps2dq    xmm7, xmm7
    pand    xmm4, [lsb32_mask wrt rip]
    pand    xmm5, [lsb32_mask wrt rip]
    pand    xmm6, [lsb32_mask wrt rip]
    pand    xmm7, [lsb32_mask wrt rip]
    pslld   xmm4, 24
    pslld   xmm5, 16
    pslld   xmm6, 8
    por     xmm4, xmm5
    por     xmm4, xmm6
    por     xmm4, xmm7
    movdqu  [rdi], xmm4

    ; adjust pointers and loop
    add rdi, 16
    add rsi, 16
    sub rdx, 16
    jg .loop

    ret

; constants
align 16
lsb32_mask	times 4 db 0xff, 0x00, 0x00, 0x00
two_fifty_five  times 4 dd 255.0

; vim:syntax=nasm64

