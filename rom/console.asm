UART_STATUS: EQU    0x40
UART_DATA:   EQU    0x41

UART_IN_EMPTY:  EQU 0x01
UART_IN_FULL:   EQU 0x02
UART_OUT_EMPTY: EQU 0x04
UART_OUT_FULL:  EQU 0x08

; Print character to console.
; Input:
;  A - character.

CONOUT:
    PUSH    AF

.CONOUT_POLL:
    IN      A, (UART_STATUS)
    AND     UART_OUT_FULL
    JP      NZ, .CONOUT_POLL

    POP     AF

    OUT     (UART_DATA), A

    RET

; Read character from console.
; Output:
;  A - character.

CONIN:
    IN      A, (UART_STATUS)
    AND     UART_IN_EMPTY
    JP      NZ, CONIN

    IN      A, (UART_DATA)

    RET

; Get console status.
; Output:
;  A = FF - ready for read.
;  A = 00 - not ready for read.

CONST:
    IN      A, (UART_STATUS)
    AND     UART_IN_EMPTY
    JP      NZ, .CONST_EMPTY

    CPL
    RET
.CONST_EMPTY:
    XOR     A
    RET


; Get console output status.
; Output:
;  A = FF - ready for write.
;  A = 00 - not ready for write.

CONOST:
    IN      A, (UART_STATUS)
    AND     UART_OUT_FULL
    JP      NZ, .CONOST_FULL

    CPL
    RET
.CONOST_FULL:
    XOR     A
    RET

; Print message to console.
; Input:
;  HL = ASCIIZ string to output.

PUTMSG:
    PUSH    AF

.PUTMSG_LOOP:
    LD      A, (HL)
    CP      0
    JP      Z, .PUTMSG_END
    CALL    CONOUT
    INC     HL
    JP      .PUTMSG_LOOP

.PUTMSG_END:
    POP     AF
    RET