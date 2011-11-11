#ifndef __ROM__H__
#define __ROM__H__

#include "machine.h"

typedef struct rom {
	uint16_t base;
	uint16_t size;
	
	unsigned char *data;
} rom_t;

int rom_create(machine_t *machine, const char *filename, uint16_t base);

#endif

