; Graphics Test for uf2s8
; Resolution: 128x128
; VRAM: 0x8000 - 0xBFFF

start:
        LI   r7, 0x80    ; VRAM Start High Byte
        LI   r6, 0x00    ; VRAM Start Low Byte
        LI   r0, 0x00    ; Current Color / Pixel Counter Low
        LI   r1, 0x00    ; Pixel Counter High

fill_loop:
        ; Use the current pixel index as the color to create a gradient
        SB   r0, [a3]    ; Store color in VRAM at [r7:r6]

        ; Increment VRAM pointer
        ADD  r6, r6, 1
        INCC r7          ; Increment high byte if low byte overflowed

            ; Increment pixel counter (we need to fill 16384 pixels)
        ADD  r0, r0, 1
        INCC r1

        ; Check if we hit 16384 (0x4000)
        LI   r2, 0x40
        CMP  r1, r2
        B    NE, fill_loop

done:
        B    AL, done    ; Loop forever