#ifndef __BOOTSTRAP__H__
#define __BOOTSTRAP__H__

#include "machine.h"

typedef struct {
	int enabled;
	uint16_t reset;
} bootstrap_t;

int bootstrap_create(machine_t *machine, uint16_t reset_vector);

#endif

