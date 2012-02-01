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

section text align=16
global CbYCrY8422_YCbCr8P422_vector:function
CbYCrY8422_YCbCr8P422_vector:
    ; argument summary...
    ; rdi = CbYCrY data size
    ; rsi = CbYCrY8422 surface
    ; rdx = destination 'Y' plane
    ; rcx = destination 'Cb' plane
    ; r8  = destination 'Cr' plane


    ; read in YCbCr data
    movdqa      xmm0, [rsi]
    movdqa      xmm4, [rsi+16]

    movdqa      xmm1, xmm0
    movdqa      xmm5, xmm4
    pand        xmm1, [u_mask wrt rip]          ; xmm1 = [u   u   u   u   ]
    pand        xmm5, [u_mask wrt rip]          ; xmm5 = [u   u   u   u   ]

    movdqa      xmm2, xmm0
    movdqa      xmm6, xmm4
    pand        xmm2, [v_mask wrt rip]          ; xmm2 = [  v   v   v   v ]
    pand        xmm6, [v_mask wrt rip]          ; xmm6 = [  v   v   v   v ]
    psrld       xmm2, 16                        ; xmm2 = [v   v   v   v   ]
    psrld       xmm6, 16                        ; xmm6 = [v   v   v   v   ]

    psrlw       xmm0, 8                         ; xmm0 = [y y y y y y y y ]
    psrlw       xmm4, 8                         ; xmm4 = [y y y y y y y y ]

    packuswb    xmm0, xmm4                      ; xmm0 = [yyyyyyyyyyyyyyyy]
    packssdw    xmm1, xmm5                      ; xmm1 = [u u u u u u u u ]
    packssdw    xmm2, xmm6                      ; xmm2 = [v v v v v v v v ]
    packuswb    xmm1, xmm1                      ; xmm1 = [uuuuuuuu........]
    packuswb    xmm2, xmm2                      ; xmm2 = [vvvvvvvv........]

    movdqa      [rdx], xmm0
    movq        [rcx], xmm1
    movq        [r8], xmm2

    add rdx, 16
    add rcx, 8
    add r8, 8

    add rsi, 32
    sub rdi, 32
    jg CbYCrY8422_YCbCr8P422_vector

    ret

align 16
u_mask  times 4 dd 0x000000ff
v_mask  times 4 dd 0x00ff0000
; vim:syntax=nasm64
