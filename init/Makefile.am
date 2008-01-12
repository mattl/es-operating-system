SUFFIXES = .img .gz 

%.img : %.elf
	$(host_alias)-objcopy -O binary $< $@

% :: %.elf
	$(srcdir)/makeScript "`pwd`" $(srcdir) $@

noinst_SCRIPTS = es

noinst_PROGRAMS = $(noinst_SCRIPTS)

noinst_DATA = es.img fat32.img

LDADD = ../os/fs/fat/libesfat.a ../os/fs/iso9660/libesiso9660.a ../os/net/libesnet.a ../os/kernel/libeskernel.a ../os/libes++/libessup++.a

AM_LDFLAGS = -v -static -Wl,--section-start,".init"=0x81000000,-static,--omagic,--cref,-Map,$@.map

AM_CPPFLAGS = -I- \
	-I../include \
	-I$(srcdir)/../include

es_SOURCES = es.cpp

es.img: es.elf
	$(host_alias)-objcopy -O binary $< $@

fat32.img: fat32.img.gz
	gunzip -c $< > $@

EXTRA_DIST = fat32.img.gz makeScript
