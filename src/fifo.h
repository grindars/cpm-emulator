#ifndef __FIFO__H__
#define __FIFO__H__

typedef struct {
	unsigned int writeptr;
	unsigned int readptr;
	unsigned int counter;
	unsigned int size;
	unsigned char data[];
} fifo_t;

fifo_t *fifo_create(unsigned int size);
void fifo_destroy(fifo_t *fifo);

int fifo_full(fifo_t *fifo);
int fifo_empty(fifo_t *fifo);

int fifo_put(fifo_t *fifo, unsigned char data);
int fifo_get(fifo_t *fifo, unsigned char *data);

#endif
