LDADD = ../../os/fs/fat/libesfat.a ../../os/kernel/libeskernel.a ../../os/libes++/libessup++.a -lfreetype

SUFFIXES = .img .gz

%.img : %.elf
	$(host_alias)-objcopy -O binary $< $@

% :: %.elf
	$(srcdir)/makeScript "`pwd`" $(srcdir) $@

TESTS_ENVIRONMENT = $(srcdir)/testLoader

noinst_SCRIPTS = main start binder location eventManager console upcallTest newlib

noinst_PROGRAMS = $(noinst_SCRIPTS)

TESTS = $(noinst_SCRIPTS)

noinst_DATA = disk.img \
	main.img \
	start.img \
	binder.img \
	location.img \
	eventManager.img \
	console.img \
	upcallTest.img \
	newlib.img

CLEANFILES = $(noinst_DATA) $(noinst_SCRIPTS)

clean-local:
	-rm -f *.map

AM_LDFLAGS = -Wl,--section-start,".init"=0x81000000,-static,--omagic,--cref,-Map,$@.map -L$(prefix)/lib

main_SOURCES = main.cpp

start_SOURCES = start.cpp

binder_SOURCES = binder.cpp

location_SOURCES = location.cpp

eventManager_SOURCES = eventManager.cpp

console_SOURCES = console.cpp

upcallTest_SOURCES = upcallTest.cpp

newlib_SOURCES = newlib.cpp

disk.img: disk.img.gz
	gunzip -c $< > $@
	vformat $@

AM_CPPFLAGS = \
	-I- \
	-I../../include \
	-I$(srcdir)/../../include \
	-I$(prefix)/include \
	-I$(prefix)/include/freetype2

EXTRA_DIST = disk.img.gz makeScript testLoader
