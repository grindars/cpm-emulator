ORG 0xFE00

BOOT:
    LD      SP, 0xFE00

    XOR     A
    CALL    SET_BANK

    CALL    DISK_INIT

    XOR     A
    LD      BC, 0
    LD      DE, 0
    LD      HL, 0
    CALL    DISK_READ
    AND     A
    JP      NZ, DISK_ERROR

    JP      0

DISK_ERROR:
    LD      HL, ERROR_MSG
    CALL    PUTMSG
.HANG:
    JP      .HANG

ERROR_MSG: db "DISK I/O ERR", 0

include 'console.asm'
include 'spi.asm'
include 'disk.asm'
include 'memory.asm'

defs 0xFFE0 - $

; Entry points
    JP  BOOT       ; Cold boot              FFE0

; Console I/O
    JP  CONOUT     ; Console output         FFE3
    JP  CONIN      ; Console input          FFE6
    JP  CONST      ; Console status         FFE9
    JP  CONOST     ; Console output status  FFEC

; Disk I/O
    JP  DISK_READ  ; Disk read              FFEF
    JP  DISK_WRITE ; Disk write             FFF2

; Memory functions
    JP  SET_BANK   ; Set memory bank        FFF5

; Utility functions
    JP  PUTMSG     ; Console string output  FFF8
    JP  DISK_TRANS ; CHS to LBA translation FFFB

defs 0x10000 - $