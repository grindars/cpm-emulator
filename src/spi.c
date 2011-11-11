#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

#include "spi.h"

static void spi_destroy(void *data) {
    spi_t *spi = data;

    disk_destroy(spi->disk);
    free(spi);
}

static int spi_decode(struct machine *machine, uint16_t addr, int is_port,
               void *data) {
    (void) machine;
    (void) data;

    return is_port && (addr == 0x80 || addr == 0x81);
}

static uint8_t spi_pread(struct machine *machine, uint16_t addr, void *data) {
    (void) machine;

    spi_t *spi = data;

    switch(addr) {
    case 0x80:
        return 0x00;

    case 0x81:
        return spi->latch;

    default:
        return 0xFF;
    }
}

static void spi_pwrite(struct machine *machine, uint16_t addr, uint8_t value,
               void *data) {
    (void) machine;

    spi_t *spi = data;

    switch(addr) {
    case 0x80:
        if(value != spi->ctrl) {
            disk_select(spi->disk, (value & 0x01) == 0x01);

            spi->ctrl = value;
        }

        break;

    case 0x81:
        spi->latch = disk_io(spi->disk, value);

        break;
    }
}

static const machine_device_callbacks_t spi_callbacks = {
    spi_destroy,
    spi_decode,
    NULL,
    NULL,
    spi_pread,
    spi_pwrite,
    NULL,
    NULL
};

int spi_create(machine_t *machine, const char *image) {
    spi_t *spi = malloc(sizeof(spi_t));

    if(spi == NULL)
        return -1;

    spi->disk = disk_create(image);

    if(spi->disk == NULL) {
        int save = errno;
        free(spi);
        errno = save;

        return -1;
    }

    spi->latch = 0;
    spi->ctrl = 0;

    if(machine_register_device(machine, &spi_callbacks, spi) == -1) {
        int save = errno;
        disk_destroy(spi->disk);
        free(spi);
        errno = save;

        return -1;
    }

    return 0;
}