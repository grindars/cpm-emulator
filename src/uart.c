#include <sys/wait.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <pty.h>
#include <string.h>
#include <limits.h>
#include <termios.h>

#include "uart.h"

#define UART_FIFO   16
#define max(a, b) ((a) > (b) ? (a) : (b))

static int fork_xterm(pid_t *pid, int *fd) {
    int master, slave;
    struct termios term;

    if(openpty(&master, &slave, NULL, NULL, NULL) == -1)
        return -1;

    if(tcgetattr(master, &term) == -1) {
        int save = errno;
        close(master);
        close(slave);
        errno = save;

        return -1;
    }

    term.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP
        | INLCR | IGNCR | ICRNL | IXON);
    term.c_oflag &= ~OPOST;
    term.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    term.c_cflag &= ~(CSIZE | PARENB);
    term.c_cflag |= CS8;

    if(tcsetattr(master, TCSANOW, &term) == -1) {
        int save = errno;
        close(master);
        close(slave);
        errno = save;

        return -1;
    }

    *pid = fork();

    if(*pid == -1) {
        int save = errno;
        close(master);
        close(slave);
        errno = save;

        return -1;
    } else if(*pid == 0) {
        char ccn[PATH_MAX];

        sprintf(ccn, "-S%s/%d", strrchr(ttyname(slave), '/') + 1, slave);

        char *const args[] = {
            "/usr/bin/xterm",
            "-title", "Machine console", "-samename",
            ccn,
            NULL
        };

        close(master);

        execv(args[0], args);

        perror("execv");

        _exit(1);
    }

    close(slave);

    *fd = master;

    return 0;
}

static void *uart_thread(void *arg) {
    uart_t *uart = arg;
    int run = 1, flush = 1;

    pid_t pid;
    int fd, status;

    if(fork_xterm(&pid, &fd) == -1) {
        perror("fork_xterm");

        return NULL;
    }

    do {
        fd_set read_set, write_set;

        FD_ZERO(&read_set);
        FD_ZERO(&write_set);

        FD_SET(uart->ev_pipe[0], &read_set);

        pthread_mutex_lock(&(uart->mutex));

        if(!fifo_full(uart->receive_fifo))
            FD_SET(fd, &read_set);

        if(!fifo_empty(uart->send_fifo)) {
            FD_SET(fd, &write_set);
        }

        pthread_mutex_unlock(&(uart->mutex));

        int ret = select(max(uart->ev_pipe[0], fd) + 1, &read_set,
                 &write_set, NULL, NULL);

        if(ret == -1 && errno == EINTR)
            continue;
        else if(ret == -1) {
            perror("UART: select");

            break;
        }

        if(FD_ISSET(uart->ev_pipe[0], &read_set)) {
            char code;

            if(read(uart->ev_pipe[0], &code, 1) != 1)
                break;

            switch(code) {
            case 0x01:
                run = 0;

                break;
            }
        }

        pthread_mutex_lock(&(uart->mutex));

        if(!fifo_full(uart->receive_fifo) && FD_ISSET(fd, &read_set)) {
            unsigned char data;

            if(read(fd, &data, 1) != 1) {
                perror("read");

                run = 0;
            } else if(flush && data == '\n') {
                flush = 0;
            } else if(!flush)
                fifo_put(uart->receive_fifo, data);
        }

        if(!fifo_empty(uart->send_fifo) && FD_ISSET(fd, &write_set)) {
            unsigned char data;

            fifo_get(uart->send_fifo, &data);

            if(write(fd, &data, 1) != 1) {
                perror("write");

                run = 0;
            }
        }

        pthread_mutex_unlock(&(uart->mutex));

    } while(run);

    close(fd);
    kill(pid, SIGTERM);
    waitpid(pid, &status, 0);

    return NULL;
}

static void uart_stop_thread(uart_t *uart) {
    fflush(stdout);

    char control = 0x01;
    if(write(uart->ev_pipe[1], &control, 1) == 1) {
        pthread_join(uart->thread, NULL);
    }
}

static void uart_destroy(void *data) {
    uart_t *uart = data;

    uart_stop_thread(uart);
    pthread_mutex_destroy(&(uart->mutex));
    close(uart->ev_pipe[0]);
    close(uart->ev_pipe[1]);
    fifo_destroy(uart->send_fifo);
    fifo_destroy(uart->receive_fifo);
    free(uart);
}

static int uart_decode(struct machine *machine, uint16_t addr, int is_port,
               void *data) {
    (void) machine;
    (void) data;

    return is_port && (addr == 0x40 || addr == 0x41);
}

static uint8_t uart_pread(struct machine *machine, uint16_t addr, void *data) {
    (void) machine;

    uart_t *uart = data;
    unsigned char byte;

    switch(addr) {
    case 0x40:
        byte = 0;

        pthread_mutex_lock(&(uart->mutex));

        if(fifo_empty(uart->receive_fifo))
            byte |= 0x01;
        else if(fifo_full(uart->receive_fifo))
            byte |= 0x02;

        if(fifo_empty(uart->send_fifo))
            byte |= 0x04;
        else if(fifo_full(uart->send_fifo))
            byte |= 0x08;

        pthread_mutex_unlock(&(uart->mutex));

        return byte;

    case 0x41:
        pthread_mutex_lock(&(uart->mutex));

        if(fifo_get(uart->receive_fifo, &byte)) {
            pthread_mutex_unlock(&(uart->mutex));

            char control = 0x02;

            write(uart->ev_pipe[1], &control, 1);
        } else
            pthread_mutex_unlock(&(uart->mutex));

        return byte;

    default:
        return 0xFF;
    }
}

static void uart_pwrite(struct machine *machine, uint16_t addr, uint8_t value,
            void *data) {
    (void) machine;

    uart_t *uart = data;

    if(addr == 0x41)
        pthread_mutex_lock(&(uart->mutex));

        if(fifo_put(uart->send_fifo, value)) {
            pthread_mutex_unlock(&(uart->mutex));

            char control = 0x02;

            write(uart->ev_pipe[1], &control, 1);
        } else
            pthread_mutex_unlock(&(uart->mutex));
}

static const machine_device_callbacks_t uart_callbacks = {
    uart_destroy,
    uart_decode,
    NULL,
    NULL,
    uart_pread,
    uart_pwrite,
    NULL,
    NULL
};

int uart_create(machine_t *machine) {
    uart_t *uart = malloc(sizeof(uart_t));

    if(uart == NULL)
        return -1;

    uart->send_fifo = fifo_create(UART_FIFO);

    if(uart->send_fifo == NULL) {
        int save = errno;
        free(uart);
        errno = save;

        return -1;
    }

    uart->receive_fifo = fifo_create(UART_FIFO);

    if(uart->receive_fifo == NULL) {
        int save = errno;
        fifo_destroy(uart->send_fifo);
        free(uart);
        errno = save;

        return -1;
    }

    if(pipe(uart->ev_pipe) == -1) {
        int save = errno;
        fifo_destroy(uart->receive_fifo);
        fifo_destroy(uart->send_fifo);
        free(uart);
        errno = save;

        return -1;
    }

    if(pthread_mutex_init(&(uart->mutex), NULL) == -1) {
        int save = errno;
        close(uart->ev_pipe[0]);
        close(uart->ev_pipe[1]);
        fifo_destroy(uart->receive_fifo);
        fifo_destroy(uart->send_fifo);
        free(uart);
        errno = save;

        return -1;
    }

    if(pthread_create(&(uart->thread), NULL, uart_thread, uart) == -1) {
        int save = errno;
        pthread_mutex_destroy(&(uart->mutex));
        close(uart->ev_pipe[0]);
        close(uart->ev_pipe[1]);
        fifo_destroy(uart->receive_fifo);
        fifo_destroy(uart->send_fifo);
        free(uart);
        errno = save;

        return -1;
    }

    if(machine_register_device(machine, &uart_callbacks, uart) == -1) {
        int save = errno;
        uart_stop_thread(uart);

        close(uart->ev_pipe[0]);
        close(uart->ev_pipe[1]);
        fifo_destroy(uart->receive_fifo);
        fifo_destroy(uart->send_fifo);
        free(uart);
        errno = save;

        return -1;
    }

    return 0;
}
