#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include "disk.h"

#define CMD_READSTATE   0x01
#define ST_BUSY 0x01
#define ST_NOMEDIA  0x02
#define ST_READONLY 0x04
#define ST_ERROR    0x08
#define CMD_QUERYGEOM   0x02
#define CMD_INIT        0x03
#define CMD_READ        0x04
#define CMD_WRITE       0x05
#define CMD_FIFOACCESS  0x06

disk_t *disk_create(const char *image) {
    disk_t *disk = malloc(sizeof(disk_t));

    if(disk == NULL)
        return NULL;

    disk->hndl = fopen(image, "r+b");
    if(disk->hndl == NULL) {
        int save = errno;

        free(disk);

        errno = save;

        return NULL;
    }

    fseek(disk->hndl, 0, SEEK_END);
    long end = (ftell(disk->hndl) + 511) & ~511;
    ftruncate(fileno(disk->hndl), end);
    rewind(disk->hndl);

    disk->sectors = end / 512;
    disk->state = STATE_IDLE;
    disk->state_reg = 0;
    return disk;
}

void disk_destroy(disk_t *disk) {
    fclose(disk->hndl);
    free(disk);
}

unsigned char disk_io(disk_t *disk, unsigned char in) {
    switch(disk->state) {
    case STATE_IDLE:
        return 0xFF;

    default:
    case STATE_ERROR:
        fprintf(stderr, "disk: I/O in error state\n");

        return 0xFF;

    case STATE_WAITCMD:
        if((disk->state_reg & ST_BUSY) == ST_BUSY &&
            in != CMD_READSTATE) {

            fputs("disk: command while busy\n", stderr);

            disk->state = STATE_ERROR;

            return 0xFF;
        }


        switch(in) {
        case CMD_READSTATE:
            disk->state = STATE_STREAD;

            break;

        case CMD_QUERYGEOM:
            disk->counter = 0;
            disk->state = STATE_QUERYGEOM;

            break;

        case CMD_INIT:
            disk->state = STATE_ERROR;

            break;

        case CMD_READ:
            disk->counter = 0;
            disk->lba = 0;
            disk->state = STATE_READ;

            break;

        case CMD_WRITE:
            disk->counter = 0;
            disk->lba = 0;
            disk->state = STATE_WRITE;

            break;

        case CMD_FIFOACCESS:
            disk->counter = 0;
            disk->state = STATE_FIFOACCESS;

            break;

        default:
            fprintf(stderr, "disk: invalid command %02X\n", in);

            disk->state = STATE_ERROR;
        }

        return 0xFF;

    case STATE_STREAD:
        {
            unsigned char ret = disk->state_reg;

            disk->state_reg &= ~(ST_ERROR);

            return ret;
        }

    case STATE_QUERYGEOM:
        switch(disk->counter++ & 3) {
        case 0:
            return disk->sectors & 0xFF;

        case 1:
            return (disk->sectors & 0xFF00) >> 8;

        case 2:
            return (disk->sectors & 0xFF0000) >> 16;

        case 3:
            return (disk->sectors & 0xFF000000) >> 24;
        }

    case STATE_FIFOACCESS:
        {
            unsigned char ret = disk->fifo[disk->counter];
            disk->fifo[disk->counter] = in;
            disk->counter = (disk->counter + 1) & 511;

            return ret;
        }

    case STATE_READ:
    case STATE_WRITE:
        switch(disk->counter++ & 3) {
        case 0:
            disk->lba = (disk->lba & ~0xFF) | in;

            break;

        case 1:
            disk->lba = (disk->lba & ~0xFF00) | (in << 8);

            break;

        case 2:
            disk->lba = (disk->lba & ~0xFF0000) | (in << 16);

            break;

        case 3:
            disk->lba = (disk->lba & ~0xFF000000) | (in << 24);

            break;
        }

        return 0xFF;
    }

}

#include <ctype.h>
#define min(a, b) ((a) < (b) ? (a) : (b))

void hexdump(const void *data, size_t size) {
        const unsigned char *bytes = data;

        for(unsigned int i = 0; i < size; i+= 16) {
                printf("%04X  ", i);

                unsigned int chunk_size = min(size - i, 16);

                for(unsigned int j = 0; j < chunk_size; j++) {
                        printf("%02X ", bytes[i + j]);

                        if(j == 7)
                                putchar(' ');
                }

                for(unsigned int j = 0; j < 16 - chunk_size; j++) {
                        fputs("   ", stdout);


                        if(j == 8)
                                putchar(' ');
                }

                putchar('|');

                for(unsigned int j = 0; j < chunk_size; j++) {
                        putchar(isprint(bytes[i + j]) ? bytes[i + j] : '.');
                }

                puts("|");
        }
}


void disk_select(disk_t *disk, int selected) {
    if(selected)
        disk->state = STATE_WAITCMD;
    else {
        switch(disk->state) {
        case STATE_READ:
            disk->lba = (disk->lba & 0xFFFF) + (disk->lba >> 16) * 36;

            if(disk->lba >= disk->sectors) {
                disk->state_reg |= ST_ERROR;

                break;
            }

            if(fseek(disk->hndl, disk->lba * 512, SEEK_SET) != 0) {
                disk->state_reg |= ST_ERROR;

                break;
            }

            if(fread(disk->fifo, 1, 512, disk->hndl) != 512) {
                disk->state_reg |= ST_ERROR;

                break;
            }

            break;

        case STATE_WRITE:
            disk->lba = (disk->lba & 0xFFFF) + (disk->lba >> 16) * 36;

            if(disk->lba >= disk->sectors) {
                disk->state_reg |= ST_ERROR;

                break;
            }

            if(fseek(disk->hndl, disk->lba * 512, SEEK_SET) != 0) {
                disk->state_reg |= ST_ERROR;

                break;
            }

            if(fwrite(disk->fifo, 1, 512, disk->hndl) != 512) {
                disk->state_reg |= ST_ERROR;

                break;
            }

            break;
        }



        disk->state = STATE_IDLE;
    }
}
