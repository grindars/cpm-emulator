#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#include "machine.h"

int main(int argc, char *argv[]) {
    machine_t *machine = machine_create();
    if(machine == NULL) {
        perror("machine_create");

        return 1;
    }

    do {
        struct timespec time;

        if(machine_run(machine, &time) == -1) {
            perror("machine_run");

            break;
        }

        if(time.tv_sec > 0 || time.tv_nsec > 0) {
            int ret;

            do {
                ret = nanosleep(&time, &time);
            } while(ret == -1 && errno == EINTR);

            if(ret == -1) {
                perror("nanosleep");

                break;
            }
        }

    } while(1);

    printf("stopping emulator\n");

    machine_destroy(machine);

    return 0;
}
