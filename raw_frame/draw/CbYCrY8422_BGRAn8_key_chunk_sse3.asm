bits 64
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
global CbYCrY8422_BGRAn8_key_chunk_sse3

CbYCrY8422_BGRAn8_key_sse3_chunk:
    ; rdi = CbYCbY8422 background data
    ; rsi = BGRAn8 key/fill data
    ; rdx = number of pixels to key

    ; load RGB pixels
    movdqa      xmm5, [rsi]         ; xmm0 = [bgrabgrabgrabgra]
    movdqa      xmm6, [rsi+16]      ; xmm1 = [bgrabgrabgrabgra]
    add         rsi, 32

    ; xmm0 = R
    movdqa      xmm0, xmm5          ; xmm0 = [bgrabgrabgrabgra]
    pslld       xmm0, 16            ; xmm0 = [ra  ra  ra  ra  ]
    pand        xmm0, [Mask4]       ; xmm0 = [r   r   r   r   ]
    movdqa      xmm4, xmm6          ; xmm4 = [bgrabgrabgrabgra]
    pslld       xmm4, 16            ; xmm4 = [ra  ra  ra  ra  ]
    pand        xmm4, [Mask4]       ; xmm4 = [r   r   r   r   ]
    packssdw    xmm0, xmm4          ; xmm0 = [r r r r r r r r ]

    ; xmm1 = G
    movdqa      xmm1, xmm5          ; xmm1 = [bgrabgrabgrabgra]
    pslld       xmm1, 8             ; xmm1 = [gra gra gra gra ]
    pand        xmm1, [Mask4]       ; xmm1 = [g   g   g   g   ]
    movdqa      xmm4, xmm6          ; xmm4 = [bgrabgrabgrabgra]
    pslld       xmm4, 8             ; xmm4 = [gra gra gra gra ]
    pand        xmm4, [Mask4]       ; xmm4 = [g   g   g   g   ]
    packssdw    xmm1, xmm4          ; xmm1 = [g g g g g g g g ]

    ; xmm2 = B
    movdqa      xmm2, xmm5          ; xmm2 = [bgrabgrabgrabgra]
    pand        xmm2, [Mask4]       ; xmm2 = [b   b   b   b   ]
    movdqa      xmm4, xmm6          ; xmm4 = [bgrabgrabgrabgra]
    pand        xmm4, [Mask4]       ; xmm4 = [b   b   b   b   ]
    packssdw    xmm2, xmm4          ; xmm2 = [b b b b b b b b ]

    ; xmm3 = A
    movdqa      xmm3, xmm5          ; xmm3 = [bgrabgrabgrabgra]
    pslld       xmm3, 24            ; xmm3 = [a   a   a   a   ]
    movdqa      xmm4, xmm6          ; xmm4 = [bgrabgrabgrabgra]
    pslld       xmm4, 24            ; xmm4 = [a   a   a   a   ]
    packssdw    xmm3, xmm4          ; xmm4 = [a a a a a a a a ]


    movdqa  xmm1, xmm0
    movdqa  xmm2, xmm0
    movdqa  xmm3, xmm0
    pand    xmm0, [R_mask]      ; xmm0 = [  r   r   r   r ]
    pand    xmm1, [G_mask]      ; xmm1 = [ g   g   g   g  ]
    pand    xmm2, [B_mask]      ; xmm2 = [b   b   b   b   ]
    pand    xmm3, [A_mask]      ; xmm3 = [   a   a   a   a]
    pslld   xmm0, 16            ; xmm0 = [r   r   r   r   ]
    pslld   xmm1, 8             ; xmm1 = [g   g   g   g   ]
    pslld   xmm3, 24            ; xmm2 = [b   b   b   b   ]

    ; conversion from RGB to YCbCr
    ; Y
    movdqa  xmm4, xmm0
    pmul

    sub rdx, ....
    jg CbYCrY8422_BGRAn8_key_sse3_chunk

; vim:syntax=nasm64
