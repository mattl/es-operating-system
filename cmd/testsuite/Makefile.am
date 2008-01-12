LDADD = ../../os/fs/fat/libesfat.a ../../os/kernel/libeskernel.a ../../os/libes++/libessup++.a

SUFFIXES = .img .gz

%.img : %.elf
	$(host_alias)-objcopy -O binary $< $@

% :: %.elf
	$(srcdir)/makeScript "`pwd`" $@

TESTS = main start binder location

noinst_SCRIPTS = $(TESTS)

noinst_PROGRAMS = $(noinst_SCRIPTS)

noinst_DATA = disk.img \
	main.img \
	start.img \
	binder.img \
	location.img

CLEANFILES = $(noinst_DATA) $(noinst_SCRIPTS)

clean-local:
	-rm -f *.map *.log

AM_LDFLAGS = -Wl,--section-start,".init"=0x81000000,-static,--omagic,--cref,-Map,$@.map

main_SOURCES = main.cpp

start_SOURCES = start.cpp

binder_SOURCES = binder.cpp

location_SOURCES = location.cpp

disk.img: disk.img.gz
	gunzip -c $< > $@
	vformat $@

AM_CPPFLAGS = \
	-I- \
	-I../../include \
	-I$(srcdir)/../../include

EXTRA_DIST = disk.img.gz makeScript
