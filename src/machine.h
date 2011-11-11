#ifndef __MACHINE__H__
#define __MACHINE__H__

#include <stdint.h>
#include <time.h>

#define NS_PER_SEC  1000000000
#define NS_PER_MS   (NS_PER_SEC / 1000)
#define CPU_CLOCK_NS    1000
#define CYCLES_PER_MS   (NS_PER_MS / CPU_CLOCK_NS * 10)

struct machine;

#ifndef IN_CORE
typedef void Z80EX_CONTEXT;
#endif

typedef struct {
    void (*destroy_cb)(void *data);

    int (*decode)(struct machine *machine, uint16_t addr, int is_port,
              void *data);

    uint8_t (*mread_cb)(struct machine *machine, uint16_t addr, int m1_state,
                   void *data);
    void (*mwrite_cb)(struct machine *machine, uint16_t addr,
              uint8_t value, void *data);

    uint8_t (*pread_cb)(struct machine *machine, uint16_t addr, void *data);
    void (*pwrite_cb)(struct machine *machine, uint16_t addr, uint8_t value,
              void *data);

    uint8_t (*intread_cb)(struct machine *machine, void *data);
    void (*cycles_cb)(struct machine *machine, int cycles, void *data);
} machine_device_callbacks_t;

typedef struct machine_device {
    struct machine_device *next;

    void *data;
    const machine_device_callbacks_t *cb;
} machine_device_t;

typedef struct machine {
    Z80EX_CONTEXT *z80;
    machine_device_t *devices;
    unsigned int cycles;
    int desync_level;
} machine_t;

extern machine_t *global_instance;

machine_t *machine_create(void);
void machine_destroy(machine_t *machine);

int machine_register_device(machine_t *machine,
             const machine_device_callbacks_t *cb,
             void *data);

int machine_run(machine_t *machine, struct timespec *taken);

void machine_sync(machine_t *machine);
void machine_desync(machine_t *machine);

#endif
