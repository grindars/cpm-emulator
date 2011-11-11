#ifndef __UART__H__
#define __UART__H__

#include <pthread.h>
#include "machine.h"
#include "fifo.h"

typedef struct {
	fifo_t *send_fifo;
	fifo_t *receive_fifo;
	int ev_pipe[2];
	pthread_t thread;
	pthread_mutex_t mutex;
} uart_t;

int uart_create(machine_t *machine);

#endif
