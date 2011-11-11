MEMORY_BANK:    EQU 0xF0

; Set memory bank.
; Input:
;  A - requested memory bank.
; PREV_BANK set to previous memory bank.
SET_BANK:
    PUSH    AF
    IN      A, (MEMORY_BANK)
    LD      (PREV_BANK), A
    POP     AF
    OUT     (MEMORY_BANK), A
    RET

PREV_BANK:  EQU 0xFC00