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
    ; rdx = number of source bytes to key
    ; rcx = global alpha

    ; 72 instructions per 4 pixels, at last count (2011/01/26)
    ; i.e. much less than what GCC came up with :)

    ; load xmm8 with global alpha
    pxor        xmm7, xmm7 
    movd        xmm8, rcx
    pshufd      xmm8, xmm8, 0x00
    pslld       xmm8, 8 ; make alpha out of 65535 instead of 255

.loop:
    movdqa      xmm0, [rsi]
    movdqa      xmm1, xmm0
    movdqa      xmm2, xmm0
    movdqa      xmm3, xmm0

    ; get alpha in XMM0
    psrld       xmm0, 24
    pand        xmm0, [lsb32_mask wrt rip]

    ; red in XMM1
    psrld       xmm1, 16
    pand        xmm1, [lsb32_mask wrt rip]

    ; green in XMM2
    psrld       xmm2, 8
    pand        xmm2, [lsb32_mask wrt rip]

    ; blue in XMM3
    pand        xmm3, [lsb32_mask wrt rip]

    ; scale RGB to 0..219
    pmulhuw     xmm1, [rgb_scale wrt rip] ; xmm1 = xmm1 * rgb_scale / 65536
    pmulhuw     xmm2, [rgb_scale wrt rip]
    pmulhuw     xmm3, [rgb_scale wrt rip]

    ; compute luma
    ; note *potential* optimization opportunity here (we multiply green by two consts)
    pmulhuw     xmm2, [gy_scale wrt rip]        ; xmm2 = xmm2 * gy_scale / 65536
    movdqa      xmm5, xmm3
    pmulhuw     xmm5, [by_scale wrt rip]        ; xmm3 = xmm3 * by_scale / 65536
    paddusw     xmm2, xmm5
    movdqa      xmm5, xmm1
    pmulhuw     xmm5, [ry_scale wrt rip]        ; xmm4 = xmm4 * ry_scale / 65536
    paddusw     xmm2, xmm5

    ; compute Cb
    psubusw     xmm3, xmm2
    pmulhuw     xmm3, [kb wrt rip]
    paddusw     xmm3, [cb_cr_offset wrt rip]

    ; compute Cr
    psubusw     xmm1, xmm2
    pmulhuw     xmm1, [kr wrt rip]
    paddusw     xmm1, [cb_cr_offset wrt rip]

    ; offset luma
    paddusw     xmm2, [luma_offset wrt rip]

    ; now we have Y/Cb/Cr/alpha vectors (xmm2, xmm3, xmm1, xmm0 respectively)
    
    ; load four source pixels
    movq        xmm4, [rdi]             ;        msb            lsb
    punpcklbw   xmm4, xmm7              ; xmm4 = [ y v y u y v y u]
    
    pshuflw     xmm5, xmm4, 0x00
    pshufhw     xmm5, xmm5, 0x00        ; xmm5 = [ u u u u u u u u]
    pand        xmm5, [lsb32_mask wrt rip]      ; xmm5 = [   u   u   u   u]

    pshuflw     xmm6, xmm4, 0xaa        ; xmm6 = [ v v v v v v v v]
    pshufhw     xmm6, xmm6, 0xaa
    pand        xmm6, [lsb32_mask wrt rip]      ; xmm6 = [   v   v   v   v]

    psrld       xmm4, 16                ; xmm4 = [   y   y   y   y]

    ; now have background in Y/Cb/Cr vectors (xmm4/xmm5/xmm6 respectively)
    
    ; alpha blending
    ; xmm8 = global alpha, multiply into source alpha
    pslld       xmm0, 8         ; make source alpha a fraction on 65536 
    pmulhuw     xmm0, xmm8      ; xmm0 = xmm0 * xmm8 / 65536 = alpha ratio


    ; key alpha scaling
    pmulhuw     xmm1, xmm0      
    pmulhuw     xmm2, xmm0
    pmulhuw     xmm3, xmm0

    ; compute complementary alpha
    movdqa      xmm9, [alpha_65536 wrt rip]
    psubd       xmm9, xmm0

    ; background alpha scaling
    pmulhuw     xmm4, xmm9
    pmulhuw     xmm5, xmm9
    pmulhuw     xmm6, xmm9

    paddusw     xmm2, xmm4
    paddusw     xmm3, xmm5
    paddusw     xmm1, xmm6

    ; repack pixels
    packssdw    xmm2, xmm7              ; xmm2 [   y   y   y   y] -> [         y y y y]
    pslld       xmm2, 8                 ; xmm2 -> [        y y y y ]
    
    packssdw    xmm3, xmm7              ; xmm3 [   u   u   u   u] -> [         u u u u]
    pand        xmm3, [lsb32_mask wrt rip]      ; xmm3 -> [           u   u]
    por         xmm2, xmm3              ; xmm2 -> [        y yu y yu]

    packssdw    xmm1, xmm7              ; xmm4 [   v   v   v   v] -> [         v v v v]
    pslld       xmm1, 16                ; xmm4 -> [         v   v  ]
    por         xmm2, xmm1              ; xmm2 -> [        yvyuyvyu]

    movq        [rdi], xmm2
    
.next:
    add rsi, 16
    add rdi, 8
    sub rdx, 16
    jg .loop

    ret

; constants - see Poynton, "Digital Video and HDTV", chapter 26
align 16
ry_scale                times 4 dd 13933
gy_scale                times 4 dd 46871
by_scale                times 4 dd 4732

kb                      times 4 dd 36124
kr                      times 4 dd 42566

alpha_65536             times 4 dd 65535

luma_offset             times 4 dd 16
cb_cr_offset            times 4 dd 128

; Scale factors
rgb_scale               times 4 dd 56284

; Bit masks
lsb32_mask              times 4 db 0xff, 0x00, 0x00, 0x00

; vim:syntax=nasm64
