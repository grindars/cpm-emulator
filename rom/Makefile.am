romsdir = $(DATADIR)/cpm
roms_DATA = rom.bin coldboot.bin
SOURCES = rom.asm console.asm spi.asm disk.asm memory.asm
MOSTLYCLEANFILES = ${roms_DATA}
EXTRA_DIST = ${SOURCES} coldboot.asm

rom.bin: ${SOURCES}
	${Z80ASM} -o $@ $<

coldboot.bin: coldboot.asm
	${Z80ASM} -o $@ $<
