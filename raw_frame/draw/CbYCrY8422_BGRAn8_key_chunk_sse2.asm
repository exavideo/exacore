; Copyright 2011 Andrew H. Armenia.
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
global CbYCrY8422_BGRAn8_key_chunk_sse2

CbYCrY8422_BGRAn8_key_chunk_sse2:
    ; rdi = CbYCbY8422 background data
    ; rsi = BGRAn8 key/fill data
    ; rdx = number of source pixels to key
    ; rcx = global alpha

    ; load xmm8 with global alpha
    movd        xmm8, rcx
    cvtdq2ps    xmm8, xmm8
    shufps      xmm8, xmm8, 0x00
    mulps       xmm8, [OneOn255]    ; xmm8 = global alpha multiplier

.loop:
    ; xmm3 = a
    movdqa      xmm3, [rsi]
    psrld       xmm3, 24
    
    ; check if anything to do, skip expensive stuff if not
    ptest       xmm3, xmm3
    jc          .next
    cvtdq2ps    xmm3, xmm3

    mulps       xmm3, xmm8  ; multiply in global alpha

    ; xmm0 = r
    movdqa      xmm0, [rsi]
    psrld       xmm0, 16
    pand        xmm0, [Mask4]
    cvtdq2ps    xmm0, xmm0

    ; xmm1 = g
    movdqa      xmm1, [rsi]
    psrld       xmm1, 8
    pand        xmm1, [Mask4]
    cvtdq2ps    xmm1, xmm1

    ; xmm2 = b
    movdqa      xmm2, [rsi]
    pand        xmm2, [Mask4]
    cvtdq2ps    xmm2, xmm2

    ; compute Y in xmm1
    mulps       xmm1, [YG_coeff wrt rip]
    movdqa      xmm4, xmm2
    mulps       xmm4, [YB_coeff wrt rip]
    addps       xmm4, xmm1
    movdqa      xmm4, xmm0
    mulps       xmm4, [YR_coeff wrt rip]
    addps       xmm4, xmm1

    ; compute Cb = (B' - Y') * scale + offset
    subps       xmm2, xmm1
    mulps       xmm2, [CB_scale wrt rip]
    addps       xmm2, [CBCR_offset wrt rip]
    
    ; compute Cr = (R' - Y') * scale + offset
    subps       xmm0, xmm1
    mulps       xmm0, [CR_scale wrt rip]
    addps       xmm0, [CBCR_offset wrt rip]

    ; xmm0 = key Cr
    ; xmm1 = key Y
    ; xmm2 = key Cb
    ; xmm3 = key alpha

    ; load 4 pixels of background CbYCrY data
    pxor        xmm7, xmm7

    ; xmm4 = vector of 4 Cr
    movq        xmm4, [rdi]
    punpcklbw   xmm4, xmm7
    pand        xmm4, [Mask4]
    cvtdq2ps    xmm4, xmm4
    shufps      xmm6, xmm6, 0xf5

    ; xmm5 = vector of 4 Y
    movq        xmm5, [rdi]
    punpcklbw   xmm5, xmm7
    psrld       xmm5, 16
    cvtdq2ps    xmm5, xmm5

    ; xmm6 = vector of 4 Cb
    movq        xmm6, [rdi]
    punpcklbw   xmm6, xmm7
    pand        xmm6, [Mask4]
    cvtdq2ps    xmm6, xmm6
    shufps      xmm6, xmm6, 0xa0

    ; xmm4 = background Cr
    ; xmm5 = background Y
    ; xmm6 = background Cb

    ; alpha blending
    ; get background alpha in xmm7 as max-key alpha in xmm3
    movdqa      xmm7, [MaxAlpha wrt rip]
    subps       xmm7, xmm3

    ; scale source Cr/Y/Cb by source alpha
    mulps       xmm0, xmm3
    mulps       xmm1, xmm3
    mulps       xmm2, xmm3

    ; scale destination Cr/Y/Cb by destination alpha
    mulps       xmm4, xmm7
    mulps       xmm5, xmm7
    mulps       xmm6, xmm7
    
    ; combine images
    addps       xmm4, xmm0
    addps       xmm5, xmm1
    addps       xmm6, xmm2

    ; divide by 255
    mulps       xmm4, [OneOn255 wrt rip]    ; xmm5 = Y output
    mulps       xmm5, [OneOn255 wrt rip]    ; xmm6 = Cb output
    mulps       xmm6, [OneOn255 wrt rip]    ; xmm7 = Cr output

    ; convert back to integer Y/Cb/Cr values
    cvtps2dq    xmm4, xmm4          
    cvtps2dq    xmm5, xmm5         
    cvtps2dq    xmm6, xmm6        

    ; xmm4 = final Cr
    ; xmm5 = final Y
    ; xmm6 = final Cb
    
    ; store as Cb-Y-Cr-Y
    pslld       xmm5, 16
    shufps      xmm4, xmm4, 0xa0
    pand        xmm4, [Cr_mask]
    pand        xmm6, [Cb_mask]
    por         xmm5, xmm4
    por         xmm5, xmm6
    packuswb    xmm5, xmm5
    movq        [rdi], xmm5

.next:
    add rsi, 16
    add rdi, 8
    sub rdx, 16
    jg .loop

    ret

; constants - see Poynton, "Digital Video and HDTV", chapter 26
align 16
YR_coeff            times 4 dd 0.1826
YG_coeff            times 4 dd 0.6142
YB_coeff            times 4 dd 0.0620
Y_offset            times 4 dd 16.0
CBCR_offset         times 4 dd 128.0
CB_scale            times 4 dd 0.47339
CR_scale            times 4 dd 0.55781
MaxAlpha            times 4 dd 255.0
; won't find this one in Poynton... it's just 1/255.0.
; NASM can't do floating point computations to get literals,
; so we have this ugly constant instead.
OneOn255            times 4 dd 0.0039215686

; Bit masks
Mask4               times 4 db 0xff, 0x00, 0x00, 0x00
Cb_mask             times 4 db 0xff, 0x00, 0x00, 0x00
Cr_mask             times 4 db 0x00, 0x00, 0xff, 0x00
; vim:syntax=nasm64
