#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "rom.h"

static void rom_destroy(void *data) {	
	rom_t *rom = data;
	
	free(rom->data);
	free(rom);
}

static int rom_decode(struct machine *machine, uint16_t addr, int is_port,
		      void *data) {
	(void) machine;
	rom_t *rom = data;
	
	return !is_port && addr >= rom->base && 
		addr - rom->base <= (int) rom->size;
}

static uint8_t rom_mread(struct machine *machine, uint16_t addr, int m1_state, 
			    void *data) {

	(void) machine;
	(void) m1_state;
	
	rom_t *rom = data;
	
	return rom->data[addr - rom->base];
}

static const machine_device_callbacks_t rom_callbacks = {
	rom_destroy,
	rom_decode,
	rom_mread,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

int rom_create(machine_t *machine, const char *filename, uint16_t base) {
	FILE *rom = fopen(filename, "rb");
	
	if(rom == NULL) {
		fprintf(stderr, "rom_create(%s): %s\n",
			filename, strerror(errno));
		
		return -1;
	}
	
	fseek(rom, 0, SEEK_END);
	size_t size = ftell(rom);
	rewind(rom);
	
	if(base + size > 0x10000) {
		fprintf(stderr, "rom_create(%s): rom does not fits in memory\n",
			filename);
		
		fclose(rom);
		
		errno = EINVAL;
		
		return -1;
	}
	
	unsigned char *rom_data = malloc(size);
	
	if(rom_data == NULL) {
		int save = errno;
		
		fclose(rom);
		
		errno = save;
		
		return -1;
	}
	
	if(fread(rom_data, 1, size, rom) != size) {
		if(!ferror(rom))
			errno = EIO;
		
		int save = errno;
		
		free(rom_data);
		fclose(rom);
		
		errno = save;
		
		return -1;
	}
	
	fclose(rom);
	
	rom_t *rom_handle = malloc(sizeof(rom_t));
	
	if(rom_handle == NULL) {
		int save = errno;
		
		free(rom_data);
		
		errno = save;
		
		return -1;
	}
	
	rom_handle->base = base;
	rom_handle->size = size;
	rom_handle->data = rom_data;
	
	if(machine_register_device(machine, &rom_callbacks, rom_handle) == -1) {
		int save = errno;
		
		free(rom_handle);
		free(rom_data);
		
		errno = save;
		
		return -1;
	}
	
	return 0;
}
