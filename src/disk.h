#ifndef __DISK__H__
#define __DISK__H__

#include <stdio.h>

#define STATE_IDLE  0
#define STATE_ERROR 1
#define STATE_WAITCMD   2
#define STATE_STREAD    3
#define STATE_QUERYGEOM 4
#define STATE_FIFOACCESS    5
#define STATE_READ      6
#define STATE_WRITE     7

typedef struct {
    int state;
    int counter;
    unsigned int sectors;
    unsigned int lba;
    FILE *hndl;
    unsigned char state_reg;
    unsigned char fifo[512];
} disk_t;

disk_t *disk_create(const char *image);
void disk_destroy(disk_t *disk);

unsigned char disk_io(disk_t *disk, unsigned char in);
void disk_select(disk_t *disk, int selected);

#endif
