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

CbYCrY8422_BGRAn8_key_sse2_chunk:
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
    ; load RGB pixels and expand to 32-bit float
    movdqa      xmm5, [rsi]         ; xmm0 = [bgrabgrabgrabgra]
    add         rsi, 16 

    ; xmm3 = A
    movdqa      xmm3, xmm5          ; xmm3 = [bgrabgrabgrabgra]
    psrld       xmm3, 24            ; xmm3 = [a   a   a   a   ]

    ; optimization: if all alpha's are zero, skip the rest
    ptest       xmm3, xmm3
    jc          .next

    cvtdq2ps    xmm3, xmm3          ; xmm3 = [a   a   a   a   ]
    mulps       xmm3, xmm8          ; global alpha factor

    ; xmm0 = R
    movdqa      xmm0, xmm5          ; xmm0 = [bgrabgrabgrabgra]
    psrld       xmm0, 16            ; xmm0 = [ra  ra  ra  ra  ]
    pand        xmm0, [Mask4 wrt rip]       ; xmm0 = [r   r   r   r   ]
    cvtdq2ps    xmm0, xmm0          ; xmm0 = [r   r   r   r   ]

    ; xmm1 = G
    movdqa      xmm1, xmm5          ; xmm1 = [bgrabgrabgrabgra]
    psrld       xmm1, 8             ; xmm1 = [gra gra gra gra ]
    pand        xmm1, [Mask4 wrt rip]       ; xmm1 = [g   g   g   g   ]
    cvtdq2ps    xmm1, xmm1          ; xmm1 = [g   g   g   g   ]

    ; xmm2 = B
    movdqa      xmm2, xmm5          ; xmm2 = [bgrabgrabgrabgra]
    pand        xmm2, [Mask4 wrt rip]       ; xmm2 = [b   b   b   b   ]
    cvtdq2ps    xmm2, xmm2          ; xmm2 = [b   b   b   b   ]

    ; compute Y
    movdqa      xmm5, xmm0
    mulps       xmm5, [YR_coeff wrt rip]
    movdqa      xmm1, xmm4
    mulps       xmm4, [YG_coeff wrt rip]
    addps       xmm5, xmm4
    movdqa      xmm2, xmm4
    mulps       xmm4, [YB_coeff wrt rip]
    addps       xmm5, xmm4
    addps       xmm5, [Y_offset wrt rip]

    ; compute Cb
    movdqa      xmm6, xmm2
    subps       xmm6, xmm5
    mulps       xmm6, [CB_scale wrt rip]
    addps       xmm6, [CBCR_offset wrt rip]
    
    ; compute Cr
    movdqa      xmm7, xmm0
    subps       xmm7, xmm5
    mulps       xmm7, [CR_scale wrt rip]
    addps       xmm7, [CBCR_offset wrt rip]

    ; now for the key,
    ; xmm3 = [a   a   a   a   ]
    ; xmm5 = [y   y   y   y   ]
    ; xmm6 = [cb  cb  cb  cb  ]
    ; xmm7 = [cr  cr  cr  cr  ] 
    ; somehow make this look like:
    ; [cb  y   cr  y   ] [cb  y   cr  y   ].

    ; load 4 pixels of background CbYCrY data
    movq        xmm0, [rdi]         ; xmm0 = [uyvyuyvy        ] (u=Cb, y=Cr)
    pxor        xmm4, xmm4
    punpcklbw   xmm0, xmm4          ; xmm0 = [u y v y u y v y ]
    movdqa      xmm0, xmm1
    movdqa      xmm0, xmm2
    pand        xmm0, [Mask4 wrt rip]       ; xmm0 = [u   v   u   v   ]
    cvtdq2ps    xmm0, xmm0
    movdqa      xmm1, xmm0
    shufps      xmm0, xmm0, 0xa0    ; xmm0 = [cb  cb  cb  cb  ]
    shufps      xmm1, xmm1, 0xf5    ; xmm1 = [cr  cr  cr  cr  ]
    psrld       xmm2, 16            ; xmm2 = [y   y   y   y   ]
    cvtdq2ps    xmm2, xmm2

    movdqa      xmm4, [MaxAlpha wrt rip]
    subps       xmm4, xmm3

    mulps       xmm0, xmm4
    mulps       xmm1, xmm4
    mulps       xmm2, xmm4

    mulps       xmm5, xmm3
    mulps       xmm6, xmm3
    mulps       xmm7, xmm3

    addps       xmm5, xmm2
    addps       xmm6, xmm0
    addps       xmm7, xmm1

    mulps       xmm5, [OneOn255 wrt rip]    ; xmm5 = Y output
    mulps       xmm6, [OneOn255 wrt rip]    ; xmm6 = Cb output
    mulps       xmm7, [OneOn255 wrt rip]    ; xmm7 = Cr output

    ; shuffle some crap around here and make CbYCrY again
    cvtps2dq    xmm5, xmm5          ; xmm5 = [y   y   y   y   ]
    packusdw    xmm5, xmm5          ; xmm5 = [y y y y y y y y ]
    cvtps2dq    xmm6, xmm6          ; xmm6 = [u   u   u   u   ]
    cvtps2dq    xmm7, xmm7          ; xmm7 = [v   v   v   v   ]
    pand        xmm6, [MaskUnusedChroma wrt rip]    ; xmm6 = [u       u       ]
    pand        xmm7, [MaskUnusedChroma wrt rip]    ; xmm7 = [v       v       ]
    packusdw    xmm6, xmm6          ; xmm6 = [u   u   u   u   ]
    packusdw    xmm7, xmm7          ; xmm7 = [v   v   v   v   ]
    psllq       xmm5, 8             ; xmm5 = [ y y y y y y y y]
    psllq       xmm7, 16            ; xmm7 = [  v   v   v   v ]
    por         xmm5, xmm6          ; xmm5 = [uy yuy yuy yuy y]
    por         xmm5, xmm7          ; xmm5 = [uyvyuyvyuyvyuyvy]
    
    movq        [rdi], xmm5         ; [rdi] = [uyvyuyvy]

.next:
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
OneOn255            times 4 dd 0.0039215686
MaxAlpha            times 4 dd 255.0
Mask4               times 4 db 0xff, 0x00, 0x00, 0x00
MaskUnusedChroma    times 2 db 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
; vim:syntax=nasm64
