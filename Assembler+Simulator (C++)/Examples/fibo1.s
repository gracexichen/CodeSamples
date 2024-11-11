start:  not r0 r1      ; r0 = not garbage value in r1
        and r0 r0 r1   ; r0 = 0
        not r1 r0      ; r1 = FF
        and r3 r0 r0   ; r3 = 0
        add r1 r1 r1   ; r1 = FE
        not r1 r1      ; r1 = 1
        and r3 r1 r1   ; r3 = 1
loop:   and r1 r0 r0   ; r1 = copy of r0
        and r0 r3 r3   ; r0 = copy of r3
        add r3 r0 r1   ; r3 = r0 + r1
        bnz loop       ; loop if result is non-zero
