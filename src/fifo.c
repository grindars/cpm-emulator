#include <stdlib.h>

#include "fifo.h"

fifo_t *fifo_create(unsigned int size) {
	fifo_t *fifo = malloc(sizeof(fifo_t) + size);

	fifo->writeptr = 0;
	fifo->readptr = 0;
	fifo->counter = 0;
	fifo->size = size;

	return fifo;
}

void fifo_destroy(fifo_t *fifo) {
	free(fifo);
}

int fifo_full(fifo_t *fifo) {
	return fifo->counter == fifo->size;
}

int fifo_empty(fifo_t *fifo) {
	return fifo->counter == 0;
}

int fifo_put(fifo_t *fifo, unsigned char data) {
	if(fifo->counter < fifo->size) {
		fifo->data[fifo->writeptr] = data;
		fifo->writeptr = (fifo->writeptr + 1) % fifo->size;
		fifo->counter++;

		return 1;
	}

	return 0;
}

int fifo_get(fifo_t *fifo, unsigned char *data) {
	if(fifo->counter > 0) {
		*data = fifo->data[fifo->readptr];
		fifo->readptr = (fifo->readptr + 1) % fifo->size;
		fifo->counter--;

		return 1;
	}

	*data = 0xFF;

	return 0;
}
