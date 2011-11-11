CMD_READSTATUS: EQU 1
CMD_QUERYGEOM:  EQU 2
CMD_INIT:       EQU 3
CMD_READ:       EQU 4
CMD_WRITE:      EQU 5
CMD_FIFOACCESS: EQU 6

ST_BUSY:        EQU 1
ST_NOMEDIA:     EQU 2
ST_READONLY:    EQU 4
ST_ERROR:       EQU 8

; Read disk status.
; Output:
;  A - status
.DISK_READSTATUS:
    LD      A, CMD_READSTATUS
    CALL    .DISK_SELECT
    CALL    SPI_IO
    CALL    SPI_IO
    CALL    .DISK_DESELECT
    RET


; Disk ready wait.
.DISK_WAITREADY:
    PUSH    AF

.DISK_WAITREADY_LOOP:
    CALL    .DISK_READSTATUS
    AND     ST_BUSY
    JP      NZ, .DISK_WAITREADY_LOOP

    POP     AF

    RET

; Disk select.
.DISK_SELECT:
    PUSH    AF
    LD      A, 1
    SPI_SELECT
    POP     AF
    RET

; Disk deselect.
.DISK_DESELECT:
    PUSH    AF
    XOR     A
    SPI_SELECT
    POP     AF
    RET

; Disk initialization.
; Uses A.
DISK_INIT:
    CALL    .DISK_WAITREADY

    CALL    .DISK_SELECT
    LD      A, CMD_INIT
    CALL    SPI_IO
    CALL    .DISK_DESELECT

    CALL    .DISK_WAITREADY

    RET

; Sector read.
; Input:
;  A - destination memory bank,
;  BC - LBA (low half),
;  DE - LBA (high half),
;  HL - destination buffer.
; Output:
;  A - result:
;   0 - completed,
;   1 - general error,
;   2 - media is not present,
;   3 - media is read only.
DISK_READ:
    PUSH    AF
    PUSH    BC
.DISK_READ_POLL:
    CALL    .DISK_READSTATUS
    LD      B, A
    AND     ST_BUSY
    JP      NZ, .DISK_READ_POLL
    LD      A, B
    AND     ST_NOMEDIA
    JP      NZ, .DISK_NOMEDIA

    CALL    .DISK_SELECT
    LD      A, CMD_READ
    CALL    SPI_IO
    LD      A, C
    CALL    SPI_IO
    LD      A, B
    CALL    SPI_IO
    LD      A, E
    CALL    SPI_IO
    LD      A, D
    CALL    SPI_IO
    CALL    .DISK_DESELECT

.DISK_READBEGIN_LOOP:
    CALL    .DISK_READSTATUS
    LD      B, A
    AND     ST_BUSY
    JP      NZ, .DISK_READBEGIN_LOOP
    LD      A, B
    AND     ST_ERROR
    JP      NZ, .DISK_IOERROR

    POP     BC
    POP     AF
    CALL    SET_BANK
    PUSH    AF
    PUSH    BC

    CALL    .DISK_SELECT
    LD      A, CMD_FIFOACCESS
    CALL    SPI_IO

    LD      BC, 512

    PUSH    HL
.DISK_READLOOP:
    XOR     A
    CALL    SPI_IO
    LD      (HL), A

    INC     HL
    DEC     BC
    XOR     A
    CP      B
    JP      NZ, .DISK_READLOOP
    CP      C
    JP      NZ, .DISK_READLOOP
    POP     HL
    SPI_SELECT

    LD      A, (PREV_BANK)
    CALL    SET_BANK

    POP     BC
    POP     AF
    ;LD      A, C
    ;CP      36
    ;JP      Z, .DISK_IOERROR
    XOR     A
    RET

; Sector write.
; Input:
;  A - source memory bank,
;  BC - LBA (low half),
;  DE - LBA (high half),
;  HL - source buffer.
; Output:
;  A - result:
;   0 - completed,
;   1 - general error,
;   2 - media is not present,
;   3 - media is read only.
DISK_WRITE:
    PUSH    AF
    PUSH    BC
.DISK_WRITE_POLL:
    CALL    .DISK_READSTATUS
    LD      B, A
    AND     ST_BUSY
    JP      NZ, .DISK_READ_POLL
    LD      A, B
    AND     ST_NOMEDIA
    JP      NZ, .DISK_NOMEDIA
    LD      A, B
    AND     ST_READONLY
    JP      NZ, .DISK_READONLY

    POP     BC
    POP     AF
    CALL    SET_BANK
    PUSH    AF
    PUSH    BC

    CALL    .DISK_SELECT
    LD      A, CMD_FIFOACCESS
    CALL    SPI_IO

    LD      BC, 512

    PUSH    HL
.DISK_WRITELOOP:
    LD      A, (HL)
    CALL    SPI_IO

    INC     HL
    DEC     BC
    XOR     A
    CP      B
    JP      NZ, .DISK_WRITELOOP
    CP      C
    JP      NZ, .DISK_WRITELOOP
    POP     HL
    SPI_SELECT

    LD      A, (PREV_BANK)
    CALL    SET_BANK

    CALL    .DISK_WAITREADY

    POP     BC

    CALL    .DISK_SELECT
    LD      A, CMD_WRITE
    CALL    SPI_IO
    LD      A, C
    CALL    SPI_IO
    LD      A, B
    CALL    SPI_IO
    LD      A, E
    CALL    SPI_IO
    LD      A, D
    CALL    SPI_IO
    CALL    .DISK_DESELECT

    PUSH    BC

.DISK_WRITEEND_LOOP:
    CALL    .DISK_READSTATUS
    LD      B, A
    AND     ST_BUSY
    JP      NZ, .DISK_WRITEEND_LOOP
    LD      A, B
    AND     ST_ERROR
    JP      NZ, .DISK_IOERROR

    POP     BC
    POP     AF

    XOR     A
    RET

.DISK_IOERROR:
    POP     BC
    POP     AF
    LD      A, 1
    RET

.DISK_NOMEDIA:
    POP     BC
    POP     AF
    LD      A, 2
    RET

.DISK_READONLY:
    POP     BC
    POP     AF
    LD      A, 3
    RET

; Input:
;  BC - sector number
;  DE - track number
; Output:
;  DE:BC - LBA.
DISK_TRANS:
;    PUSH    HL
;    PUSH    BC
;    LD      A, 36
;    LD      HL, 0
;    CALL    mul_12
;    POP     BC
;    ADD     HL, BC
;    LD      B, H
;    LD      C, L
;    LD      DE, 0
;    POP     HL
    RET
