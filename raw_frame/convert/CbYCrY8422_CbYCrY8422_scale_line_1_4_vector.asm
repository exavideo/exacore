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
global CbYCrY8422_CbYCrY8422_scale_line_1_4_vector:function

CbYCrY8422_CbYCrY8422_scale_line_1_4_vector:
    ; argument summary...
    ; rdi = CbYCrY data size
    ; rsi = CbYCrY8422 surface
    ; rdx = destination CbYCrY8422 surface
    ; U = Cb, V = Cr
                                        ;        LSB ---------- MSB
    pinsrw      xmm0, [rsi], 0          ; xmm0 = [uy              ]
    pinsrw      xmm0, [rsi+10], 1       ; xmm0 = [uyvy            ]
    pinsrw      xmm0, [rsi+16], 2       ; xmm0 = [uyvyuy          ]
    pinsrw      xmm0, [rsi+26], 3       ; xmm0 = [uyvyuyvy        ]
    pinsrw      xmm0, [rsi+32], 4       ; xmm0 = [uyvyuyvyuy      ]
    pinsrw      xmm0, [rsi+42], 5       ; xmm0 = [uyvyuyvyuyvy    ]
    pinsrw      xmm0, [rsi+48], 6       ; xmm0 = [uyvyuyvyuyvyuy  ]
    pinsrw      xmm0, [rsi+58], 7       ; xmm0 = [uyvyuyvyuyvyuyvy]

    movdqa      [rdx], xmm0

    ; book keeping
    add         rdx, 16
    add         rsi, 64 
    sub         rdi, 64 
    jg CbYCrY8422_CbYCrY8422_scale_line_1_4_vector

    ret

; vim:syntax=nasm64
