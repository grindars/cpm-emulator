#ifndef __RAM__H__
#define __RAM__H__

#include "machine.h"

typedef struct ram {
    unsigned char *data;
    uint16_t base;
    uint16_t common_base;
    unsigned int size;
    int bank_count;
    int active_bank;
} ram_t;

int ram_create(machine_t *machine, uint16_t base, uint16_t common_base,
               unsigned int size, int bank_count);

#endif

