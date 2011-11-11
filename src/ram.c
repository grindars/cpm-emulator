#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "ram.h"

static void ram_destroy(void *data) {
    ram_t *ram = data;

    free(ram->data);
    free(ram);
}

static int ram_decode(struct machine *machine, uint16_t addr, int is_port,
              void *data) {
    (void) machine;
    ram_t *ram = data;

    return
        (!is_port && addr >= ram->base && addr - ram->base <= (int) ram->size) ||
        (is_port && addr == 0xF0);
}

static uint8_t ram_mread(struct machine *machine, uint16_t addr, int m1_state,
                void *data) {

    (void) machine;
    (void) m1_state;

    ram_t *ram = data;

    unsigned int bank = ram->active_bank;
    addr -= ram->base;
    if(addr >= ram->common_base)
        bank = 0;

    return ram->data[addr + ram->size * bank];
}

static void ram_mwrite(struct machine *machine, uint16_t addr, uint8_t value,
               void *data) {

    (void) machine;

    ram_t *ram = data;

    unsigned int bank = ram->active_bank;
    addr -= ram->base;
    if(addr >= ram->common_base)
        bank = 0;

    ram->data[addr + ram->size * bank] = value;
}

static uint8_t ram_pread(struct machine *machine, uint16_t addr, void *data) {
    (void) machine;
    (void) addr;

    ram_t *ram = data;

    return ram->active_bank;
}

static void ram_pwrite(struct machine *machine, uint16_t addr, uint8_t value,
              void *data) {
    (void) machine;
    (void) addr;

    ram_t *ram = data;

    if(value >= ram->bank_count) {
        ram->active_bank = 0;

        fprintf(stderr, "Unsupported bank %hhu\n", value);
    } else if(ram->active_bank != value) {
        ram->active_bank = value;
    }
}

static const machine_device_callbacks_t ram_callbacks = {
    ram_destroy,
    ram_decode,
    ram_mread,
    ram_mwrite,
    ram_pread,
    ram_pwrite,
    NULL,
    NULL
};

int ram_create(machine_t *machine, uint16_t base, uint16_t common_base,
               unsigned int size, int bank_count) {
    if(bank_count <= 0 || size == 0) {
        fputs("ram_create: ram cannot be empty\n", stderr);

        errno = EINVAL;

        return -1;
    }

    if(base + size > 0x10000) {
        fputs("ram_create: ram does not fits in memory\n", stderr);

        errno = EINVAL;

        return -1;
    }

    unsigned char *ram_data = malloc(size * bank_count);

    if(ram_data == NULL)
        return -1;


    ram_t *ram = malloc(sizeof(ram_t));

    if(ram == NULL) {
        int save = errno;

        free(ram_data);

        errno = save;

        return -1;
    }

    ram->data = ram_data;
    ram->base = base;
    ram->size = size;
    ram->bank_count = bank_count;
    ram->active_bank = 0;
    ram->common_base = common_base;

    if(machine_register_device(machine, &ram_callbacks, ram) == -1) {
        int save = errno;

        free(ram);
        free(ram_data);

        errno = save;

        return -1;
    }

    return 0;
}
