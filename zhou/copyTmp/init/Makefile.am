SUFFIXES = .img .gz 

%.img : %.elf
	$(host_alias)-objcopy -O binary $< $@

% :: %.elf
	$(srcdir)/makeScript "`pwd`" $(srcdir) $@

noinst_SCRIPTS = es

noinst_PROGRAMS = $(noinst_SCRIPTS)

noinst_DATA = es.img fat32.img

LDADD = ../os/fs/fat/libesfat.a ../os/fs/iso9660/libesiso9660.a ../os/net/libesnet.a ../os/kernel/libeskernel.a ../os/libes++/libessup++.a

AM_LDFLAGS = -v -static -Wl,--section-start,".init"=0x81000000,-static,--no-omagic,--cref,-Map,$@.map

AM_CPPFLAGS = -I- \
	-I../include \
	-I$(srcdir)/../include \
	-I$(srcdir)/../os/fs/fat \
	-I$(srcdir)/../os/fs/iso9660

es_SOURCES = es.cpp

es.img: es.elf
	$(host_alias)-objcopy -O binary $< $@

fat32.img: fat32.img.gz
	gunzip -c $< > $@

es.elf : makeScript

EXTRA_DIST = fat32.img.gz makeScript 40-generic.conf fonts.conf fonts.dtd 1b68cf4fced22970e89022fed6868d03-x86.cache-2

clean-local:
	-rm -f es es.elf.map fat32.img

