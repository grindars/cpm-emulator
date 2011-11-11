#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

#include "bootstrap.h"

static void bootstrap_destroy(void *data) {
	bootstrap_t *bootstrap = data;
	
	free(bootstrap);
}

static int bootstrap_decode(struct machine *machine, uint16_t addr, int is_port, void *data) {
	(void) machine;
	
	bootstrap_t *bootstrap = data;
	
	return bootstrap->enabled && !is_port;
}

static uint8_t bootstrap_mread(struct machine *machine, uint16_t addr, int m1_state,
			          void *data) {

	(void) machine;
	(void) m1_state;
	
	bootstrap_t *bootstrap = data;
	
	switch(addr & 0x03) {
	case 0x00:
		return 0xC3;
	
	case 0x01:
		return bootstrap->reset & 0xFF;
	
	case 0x02:
		bootstrap->enabled = 0;
		
		return bootstrap->reset >> 8;
		
	default:
	case 0x03:
		return 0xC7;
	}
}


static const machine_device_callbacks_t bootstrap_callbacks = {
	bootstrap_destroy,
	bootstrap_decode,
	bootstrap_mread,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

int bootstrap_create(machine_t *machine, uint16_t reset_vector) {
	bootstrap_t *bootstrap = malloc(sizeof(bootstrap_t));
	
	if(bootstrap == NULL)
		return -1;
	
	bootstrap->enabled = 1;
	bootstrap->reset = reset_vector;
	
	if(machine_register_device(machine, &bootstrap_callbacks, bootstrap) == -1) {
		int save = errno;
		
		free(bootstrap);
		
		errno = save;
		
		return -1;
	}
	
	return 0;
}
