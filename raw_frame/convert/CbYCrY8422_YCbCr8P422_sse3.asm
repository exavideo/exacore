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

section text align=16
global CbYCrY8422_YCbCr8P422_sse3

CbYCrY8422_YCbCr8P422_sse3:
    ; argument summary...
    ; rdi = packed data size
    ; rsi = packed data pointer
    ; rdx = Y plane pointer
    ; rcx = Cb plane pointer
    ; r8 = Cr plane pointer
    
    movdqa xmm0, [rsi]          ; xmm0 = [uyvyuyvyuyvyuyvy]
    movdqa xmm3, [rsi+16]
    movdqa xmm6, [rsi+32]
    movdqa xmm9, [rsi+48]

    add rsi, 64
   
    movdqa xmm1, xmm0
    movdqa xmm2, xmm0
    movdqa xmm4, xmm3
    movdqa xmm5, xmm3
    movdqa xmm7, xmm6
    movdqa xmm8, xmm6
    movdqa xmm10, xmm9
    movdqa xmm11, xmm9

    pand xmm0, [Y_mask wrt rip]         ; xmm0 = [ y y y y y y y y]
    pand xmm3, [Y_mask wrt rip]
    pand xmm6, [Y_mask wrt rip]
    pand xmm9, [Y_mask wrt rip]
    pand xmm1, [Cb_mask wrt rip]        ; xmm1 = [u   u   u   u   ]
    pand xmm4, [Cb_mask wrt rip]
    pand xmm7, [Cb_mask wrt rip]
    pand xmm10, [Cb_mask wrt rip]
    pand xmm2, [Cr_mask wrt rip]        ; xmm2 = [  v   v   v   v ]
    pand xmm5, [Cr_mask wrt rip]
    pand xmm8, [Cr_mask wrt rip]
    pand xmm11, [Cr_mask wrt rip]

    psrlw xmm0, 8               ; xmm0 = [y y y y y y y y ]
    psrlw xmm3, 8
    psrlw xmm6, 8
    psrlw xmm9, 8

    psrld xmm2, 16              ; xmm2 = [v   v   v   v   ]
    psrld xmm5, 16
    psrld xmm8, 16
    psrld xmm11, 16

    packuswb xmm0, xmm3         ; xmm0 = [yyyyyyyyyyyyyyyy]
    packuswb xmm6, xmm9

    packuswb xmm2, xmm5         ; xmm2 = [v v v v v v v v ]
    packuswb xmm8, xmm11
    packuswb xmm1, xmm4         ; xmm1 = [u u u u u u u u ]
    packuswb xmm7, xmm10

    packuswb xmm2, xmm8         ; xmm2 = [vvvvvvvvvvvvvvvv]
    packuswb xmm1, xmm7         ; xmm1 = [uuuuuuuuuuuuuuuu]
    
    movdqa [rdx], xmm0            ; [rdx] = [yyyyyyyyyyyyyyyy]
    movdqa [rdx+16], xmm6
    movdqa [rcx], xmm1            ; [rcx] = [uuuuuuuu]
    movdqa [r8], xmm2             ; [r8] = [vvvvvvvv]

    add rdx, 32
    add rcx, 16
    add r8, 16

    sub rdi, 64
    jg CbYCrY8422_YCbCr8P422_sse3

    ret

align 16
Y_mask:
    times 8 db 0x00, 0xff
Cb_mask:
    times 4 db 0xff, 0x00, 0x00, 0x00
Cr_mask:
    times 4 db 0x00, 0x00, 0xff, 0x00


; vim:syntax=nasm64
