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

; vim:syntax=nasm64

bits 64

section .text align=16
global YCbCr8P422_CbYCrY8422_vector:function
YCbCr8P422_CbYCrY8422_vector:
    ; argument summary...
    ; rdi = CbYCrY data size
    ; rsi = CbYCrY8422 surface
    ; rdx = destination 'Y' plane
    ; rcx = destination 'Cb' plane
    ; r8  = destination 'Cr' plane

    pxor        xmm7, xmm7

    ; read in Y, Cb, Cr
    movdqa      xmm0, [rdx]
    movdqa      xmm4, xmm0
    movq        xmm1, [rcx]     ; xmm1 = [uuuuuuuu........]
    movq        xmm2, [r8]
    
    punpcklbw   xmm0, xmm7      ; xmm0 = [y y y y y y y y ]
    punpckhbw   xmm4, xmm7      ; xmm4 = [y y y y y y y y ]
    psllw       xmm0, 8         ; xmm0 = [ y y y y y y y y]
    psllw       xmm4, 8         ; xmm4 = [ y y y y y y y y]

    punpcklbw   xmm1, xmm7      ; xmm1 = [u u u u u u u u ]
    movdqa      xmm5, xmm1      ; xmm1 = [u u u u u u u u ]
    punpcklwd   xmm1, xmm7      ; xmm1 = [u   u   u   u   ]
    punpckhwd   xmm5, xmm7      ; xmm1 = [u   u   u   u   ]

    por         xmm0, xmm1      ; xmm0 = [uy yuy yuy yuy y]
    por         xmm4, xmm5      ; xmm4 = [uy yuy yuy yuy y]

    punpcklbw   xmm2, xmm7      ; xmm2 = [v v v v v v v v ]
    movdqa      xmm6, xmm2      ; xmm6 = [v v v v v v v v ]
    punpcklwd   xmm2, xmm7      ; xmm2 = [v   v   v   v   ]
    punpckhwd   xmm6, xmm7      ; xmm6 = [v   v   v   v   ]

    pslld       xmm2, 16        ; xmm2 = [  v   v   v   v ]
    pslld       xmm6, 16        ; xmm6 = [  v   v   v   v ]
    
    por         xmm0, xmm2      ; xmm0 = [uyvyuyvyuyvyuyvy]
    por         xmm4, xmm6      ; xmm6 = [uyvyuyvyuyvyuyvy]

    movdqa      [rsi], xmm0
    movdqa      [rsi+16], xmm4

    add rdx, 16
    add rcx, 8
    add r8, 8

    add rsi, 32
    sub rdi, 32
    jg YCbCr8P422_CbYCrY8422_vector

    ret

; vim:syntax=nasm64
