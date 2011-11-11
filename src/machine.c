#define IN_CORE
#include <z80ex/z80ex.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "machine.h"
#include "ram.h"
#include "bootstrap.h"
#include "rom.h"
#include "uart.h"
#include "spi.h"

static Z80EX_BYTE machine_mread(Z80EX_CONTEXT *cpu,
                 Z80EX_WORD addr,
                 int m1_state,
                 void *user_data) {

    machine_t *machine = user_data;
    machine_device_t *device;


    for(device = machine->devices; device; device = device->next) {
        if(device->cb->decode == NULL ||
           !device->cb->decode(machine, addr, 0, device->data))
            continue;

        if(device->cb->mread_cb) {
            return device->cb->mread_cb(machine, addr, m1_state, device->data);
        } else
            return 0xFF;
    }

    fprintf(stderr, "machine: unhandled memory read from %04hX, m1 = %d\n",
        addr, m1_state);

    return 0xFF;
}

static void machine_mwrite(Z80EX_CONTEXT *cpu,
            Z80EX_WORD addr,
            Z80EX_BYTE value,
            void *user_data) {

    machine_t *machine = user_data;
    machine_device_t *device;

    for(device = machine->devices; device; device = device->next) {
        if(device->cb->decode == NULL ||
           !device->cb->decode(machine, addr, 0, device->data))
            continue;

        if(device->cb->mwrite_cb)
            device->cb->mwrite_cb(machine, addr, value, device->data);

        return;
    }


    fprintf(stderr, "machine: unhandled memory write %02hhX to %04hX\n",
        value, addr);
}

static Z80EX_BYTE machine_pread(Z80EX_CONTEXT *cpu,
                     Z80EX_WORD port,
                     void *user_data) {
    machine_t *machine = user_data;
    machine_device_t *device;

    port &= 0x00FF;

    for(device = machine->devices; device; device = device->next) {
        if(device->cb->decode == NULL ||
           !device->cb->decode(machine, port, 1, device->data))
            continue;

        if(device->cb->pread_cb)
            return device->cb->pread_cb(machine, port, device->data);
        else
            return 0xFF;
    }

    fprintf(stderr, "machine: unhandled port read from %02hX\n", port);

    return 0xFF;
}

static void machine_pwrite(Z80EX_CONTEXT *cpu,
            Z80EX_WORD port,
            Z80EX_BYTE value,
            void *user_data) {

    machine_t *machine = user_data;
    machine_device_t *device;

    port &= 0x00FF;

    for(device = machine->devices; device; device = device->next) {
        if(device->cb->decode == NULL ||
           !device->cb->decode(machine, port, 1, device->data))
            continue;

        if(device->cb->pwrite_cb)
            device->cb->pwrite_cb(machine, port, value, device->data);

        return;
    }

    fprintf(stderr, "machine: unhandled port write %02hhX to %02hX\n", value, port);
}

static Z80EX_BYTE machine_intread(Z80EX_CONTEXT *cpu, void *user_data) {
    machine_t *machine = user_data;
    machine_device_t *device;

    for(device = machine->devices; device; device = device->next) {
        if(device->cb->intread_cb)
            return device->cb->intread_cb(machine, device->data);
    }

    fprintf(stderr, "machine: unhandled interrupt read\n");

    return 0xFF;
}

machine_t *machine_create(void) {
    machine_t *machine = malloc(sizeof(machine_t));

    if(machine == NULL)
        return NULL;

    machine->z80 = z80ex_create(
        machine_mread, machine,
        machine_mwrite, machine,
        machine_pread, machine,
        machine_pwrite, machine,
        machine_intread, machine);

    if(machine->z80 == NULL) {
        int save = errno;

        free(machine);

        errno = save;
        return NULL;
    }

    machine->devices = NULL;
    machine->cycles = 0;
    machine->desync_level = 0;

    if(ram_create(machine, 0x0000, 0xC000, 0x10000, 16) == -1) {
        int save = errno;

        machine_destroy(machine);

        errno = save;

        return NULL;
    }

    if(bootstrap_create(machine, 0xFE00) == -1) {
        int save = errno;

        machine_destroy(machine);

        errno = save;

        return NULL;
    }

    if(rom_create(machine, "../rom/rom.bin", 0xFE00) == -1) {
        int save = errno;

        machine_destroy(machine);

        errno = save;

        return NULL;
    }

    if(uart_create(machine) == -1) {
        int save = errno;

        machine_destroy(machine);

        errno = save;

        return NULL;
    }

    if(spi_create(machine, "disk.img") == -1) {
        int save = errno;

        machine_destroy(machine);

        errno = save;

        return NULL;
    }

    return machine;
}

void machine_destroy(machine_t *machine) {
    machine_device_t *i, *n;

    for(i = machine->devices; i; i = n) {
        n = i->next;

        if(i->cb->destroy_cb)
            i->cb->destroy_cb(i->data);

        free(i);
    }

    z80ex_destroy(machine->z80);
    free(machine);
}

int machine_run(machine_t *machine, struct timespec *taken) {
    machine_device_t *device;

    int step_cycles = z80ex_step(machine->z80);

    if(step_cycles) {
        machine->cycles += step_cycles;

        for(device = machine->devices; device; device = device->next)
            if(device->cb->cycles_cb)
                device->cb->cycles_cb(machine, step_cycles,
                              device->data);
    }

    memset(taken, 0, sizeof(struct timespec));

    while(machine->cycles >= CYCLES_PER_MS) {
        if(machine->desync_level == 0) {
            taken->tv_nsec += NS_PER_MS;

            if(taken->tv_nsec >= NS_PER_SEC) {
                taken->tv_sec++;
                taken->tv_nsec -= NS_PER_SEC;
            }
        }

        machine->cycles -= CYCLES_PER_MS;
    }

    return 0;
}

int machine_register_device(machine_t *machine,
             const machine_device_callbacks_t *cb,
             void *data) {

    machine_device_t *device = malloc(sizeof(machine_device_t));

    if(device == NULL)
        return -1;

    device->next = machine->devices;
    device->cb = cb;
    device->data = data;
    machine->devices = device;

    return 0;
}

void machine_sync(machine_t *machine) {
    if(--machine->desync_level == 0) {
        printf("machine: entering sync mode\n");
    }
}

void machine_desync(machine_t *machine) {
    if(++machine->desync_level == 1) {
        printf("machine: entering desync mode\n");
    }
}
