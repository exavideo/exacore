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

section .text align=16
global CbYCrY8422_BGRAn8_scale_line_1_2_vector:function

CbYCrY8422_BGRAn8_scale_line_1_2_vector:
    ; argument summary...
    ; rdi = CbYCrY data size
    ; rsi = CbYCrY8422 surface
    ; rdx = destination BGRAn8 surface
    ; U = Cb, V = Cr
    
    pxor   xmm7, xmm7
    
                                        ;        LSB ---------- MSB
    pinsrw      xmm0, [rsi], 0          ; xmm0 = [uy              ]
    pinsrw      xmm0, [rsi+2], 1        ; xmm0 = [uyvy            ]
    pinsrw      xmm0, [rsi+8], 2        ; xmm0 = [uyvyuy          ]
    pinsrw      xmm0, [rsi+10], 3       ; xmm0 = [uyvyuyvy        ]
    pinsrw      xmm0, [rsi+16], 4       ; xmm0 = [uyvyuyvyuy      ]
    pinsrw      xmm0, [rsi+18], 5       ; xmm0 = [uyvyuyvyuyvy    ]
    pinsrw      xmm0, [rsi+24], 6       ; xmm0 = [uyvyuyvyuyvyuy  ]
    pinsrw      xmm0, [rsi+26], 7       ; xmm0 = [uyvyuyvyuyvyuyvy]


    pshuflw     xmm1, xmm0, 0xa0        ; xmm1 = [uyuyuyuyuyvyuyvy]
    pshufhw     xmm1, xmm1, 0xa0        ; xmm1 = [uyuyuyuyuyuyuyuy]
    pand        xmm1, [lsb_mask wrt rip]        ; xmm1 = [u u u u u u u u ]

    pshuflw     xmm2, xmm0, 0xf5        ; xmm2 = [vyvyvyvyuyvyuyvy]
    pshufhw     xmm2, xmm2, 0xf5        ; xmm2 = [vyvyvyvyvyvyvyvy]
    pand        xmm2, [lsb_mask wrt rip]        ; xmm2 = [v v v v v v v v ]

    psrlw       xmm0, 8                 ; xmm0 = [y y y y y y y y ]
    psubusw     xmm0, [Y_shift wrt rip]         ; xmm0 -= 16
    
    ; compute R and B components

    psllw       xmm1, 1                         ; xmm1 *= 2
    psllw       xmm2, 1                         ; xmm2 *= 2
    pmulhuw     xmm1, [Cb_B_mult wrt rip]       ; xmm1 *= 59447/65536
    pmulhuw     xmm2, [Cr_R_mult wrt rip]       ; xmm2 *= xmm2 * 50451/65536

    paddusw     xmm1, xmm0              ; xmm1 += xmm0 (Y') 
    paddusw     xmm2, xmm0              ; xmm2 += xmm0 (Y')
    psubusw     xmm1, [B_offset wrt rip]        ; xmm1 -= 232
    psubusw     xmm2, [R_offset wrt rip]        ; xmm2 -= 197

    ; compute G component
    movdqa      xmm4, xmm1
    pmulhuw     xmm4, [BG_mult wrt rip]         ; xmm4 = Kb * B
    psubusw     xmm0, xmm4              ; xmm0 = Y - Kb*B
    movdqa      xmm4, xmm2
    pmulhuw     xmm4, [RG_mult wrt rip]         ; xmm4 = Kr * R
    psubusw     xmm0, xmm4              ; xmm0 = Y - Kb*B - Kr * R
    psllw       xmm0, 1
    pmulhuw     xmm0, [G_scale wrt rip]         ; xmm1 = (Y - Kb*B - Kr * R) * (1/(1-Kb-Kr)) = G

    psllw       xmm0, 1
    psllw       xmm1, 1
    psllw       xmm2, 1
    pmulhuw     xmm0, [RGB_scale wrt rip]       ; scale 0-219 -> 0-255
    pmulhuw     xmm1, [RGB_scale wrt rip]
    pmulhuw     xmm2, [RGB_scale wrt rip]

    ; RGB saturation logic
    packuswb    xmm0, xmm7
    packuswb    xmm1, xmm7
    packuswb    xmm2, xmm7

    punpcklbw   xmm0, xmm7
    punpcklbw   xmm1, xmm7
    punpcklbw   xmm2, xmm7

    ; pack down xmm0/xmm1/xmm2 to something resembling RGBA
    movdqa      xmm3, xmm0
    punpcklwd   xmm0, xmm7              ; xmm0 = [g   g   g   g   ]
    punpckhwd   xmm3, xmm7              ; xmm3 = [g   g   g   g   ]
    pslld       xmm0, 8                 ; xmm0 = [ g   g   g   g  ]
    pslld       xmm3, 8                 ; xmm3 = [ g   g   g   g  ]

    movdqa      xmm4, xmm1              
    punpcklwd   xmm1, xmm7              ; xmm1 = [b   b   b   b   ]
    punpckhwd   xmm4, xmm7              ; xmm4 = [b   b   b   b   ]
    por         xmm0, xmm1
    por         xmm3, xmm4

    movdqa      xmm5, xmm2              
    punpcklwd   xmm2, xmm7              ; xmm2 = [r   r   r   r   ]
    punpckhwd   xmm5, xmm7              ; xmm5 = [r   r   r   r   ]
    pslld       xmm2, 16
    pslld       xmm5, 16
    por         xmm0, xmm2
    por         xmm3, xmm5
    
    por         xmm0, [alpha_fixed wrt rip]
    por         xmm3, [alpha_fixed wrt rip]

    movdqa      [rdx], xmm0
    movdqa      [rdx+16], xmm3

    ; book keeping
    add         rdx, 32
    add         rsi, 32 
    sub         rdi, 32 
    jg CbYCrY8422_BGRAn8_scale_line_1_2_vector

    ret

align 16
lsb_mask        times 8 dw 0xff
Y_shift         times 8 dw 16
Cb_B_mult       times 8 dw 59447
Cr_R_mult       times 8 dw 50451
B_offset        times 8 dw 232
R_offset        times 8 dw 197
BG_mult         times 8 dw 4731
RG_mult         times 8 dw 13933
G_scale         times 8 dw 45817
alpha_fixed     times 4 dd 0xff000000
RGB_scale       times 8 dw 38154
; vim:syntax=nasm64
