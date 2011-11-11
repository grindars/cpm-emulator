SPI_STATUS:  EQU    0x80
SPI_SSEL:    EQU    0x80
SPI_DATA:    EQU    0x81

SPI_STATUS_BUSY: EQU 0x01

; SPI Device Select.
; In:
;  A = 0 - no device selected.
;  A = peripheral id.

SPI_SELECT: macro
    OUT     (SPI_SSEL), A
endm

; SPI I/O.
; In:
;  A - output byte.
; Out:
;  A - input byte.

SPI_IO:
    OUT     (SPI_DATA), A

.SPI_IO_BUSY:
    IN      A, (SPI_STATUS)
    AND     SPI_STATUS_BUSY
    JP      NZ, .SPI_IO_BUSY

    IN      A, (SPI_DATA)

    RET
