bin_PROGRAMS = cpm
cpm_SOURCES  = main.c machine.h machine.c ram.h ram.c bootstrap.h bootstrap.c \
	rom.h rom.c fifo.h fifo.c uart.h uart.c spi.h spi.c disk.h disk.c
cpm_CPPFLAGS = -DROMS_PATH=\"$(datadir)/cpm/\"
cpm_CFLAGS = --std=gnu99 -Wall -W -Wno-unused-parameter
cpm_LDFLAGS = -lpthread -lutil
