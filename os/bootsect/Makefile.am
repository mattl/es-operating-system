EXEEXT =

bin_PROGRAMS = bootfat12.bin bootfat16.bin bootfat32.bin es.ldr mbr.bin

noinst_HEADERS = fat.h

bootfat12_bin_CCASFLAGS = -DFAT12
bootfat16_bin_CCASFLAGS = -DFAT16
bootfat32_bin_CCASFLAGS = -DFAT32

bootfat12_bin_SOURCES = bootfat.S
bootfat16_bin_SOURCES = bootfat.S
bootfat32_bin_SOURCES = bootfat32.S
es_ldr_SOURCES = loader.S
mbr_bin_SOURCES = mbr.S

INCLUDES = \
	-I$(srcdir)

# XXX cross-compile only
LINK = $(host_alias)-ld -Ttext 0 -s --oformat binary -o $@ -Map $@.map

clean-local:
	-rm -f *.map

EXTRA_DIST = mbr.bin bootfat12.bin bootfat32.bin bootfat16.bin
