#ifndef __SPI__H__
#define __SPI__H__

#include "machine.h"
#include "disk.h"

typedef struct {
    disk_t *disk;
    unsigned char latch;
    unsigned char ctrl;
} spi_t;

int spi_create(machine_t *machine, const char *image);

#endif
