ORG 0

PUTMSG:    EQU 0xFFF8
DISK_READ: EQU 0xFFEF
    LD      HL, 0x100
    LD      BC, 0x01
    LD      DE, 16

.READ_LOOP:
    PUSH    DE
    LD      A, 0
    LD      DE, 0
    CALL    DISK_READ
    LD      DE, 512
    ADD     HL, DE
    POP     DE
    AND     A
    JP      NZ, .IO_ERROR

    INC     BC
    DEC     DE
    XOR     A
    CP      D
    JP      NZ, .READ_LOOP
    CP      E
    JP      NZ, .READ_LOOP

    JP      0x100

.IO_ERROR:
    LD      HL, ERROR_MSG
    CALL    PUTMSG
.HANG: JP .HANG

ERROR_MSG: DB "I/O Error\r\n", 0