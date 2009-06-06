LDADD = ../libesiso9660.a ../../../kernel/libeskernel.a ../../../libes++/libessup++.a

SUFFIXES = .img .gz .iso

%.img : %.elf
	$(host_alias)-objcopy -O binary $< $@
	$(srcdir)/makeScript "`pwd`" $(*F)

if POSIX

TESTS = iso9660 iso9660_create iso9660_attribute iso9660_readwrite iso9660_read \
        iso9660_maxLevel iso9660_getStream \
        iso9660_object iso9660_remove iso9660_size iso9660_time iso9660_rename

noinst_PROGRAMS = $(TESTS)

noinst_DATA = isotest.iso fat16_5MB.img.gz

if APPLE
LDADD += -lpthread
else
LDADD += -lpthread -lrt
endif

endif POSIX

if ES

TESTS_ENVIRONMENT = $(srcdir)/testLoader

TESTS = iso9660 iso9660_create iso9660_attribute iso9660_readwrite iso9660_read \
        iso9660_maxLevel iso9660_getStream \
        iso9660_object iso9660_remove iso9660_size iso9660_time iso9660_rename

noinst_SCRIPTS = $(TESTS)

noinst_PROGRAMS = $(noinst_SCRIPTS)

noinst_DATA = isotest.iso fat16_5MB.img \
	iso9660.img iso9660_create.img iso9660_attribute.img iso9660_readwrite.img iso9660_read.img \
        iso9660_maxLevel.img iso9660_getStream.img \
        iso9660_object.img iso9660_remove.img iso9660_size.img iso9660_time.img iso9660_rename.img

CLEANFILES = $(noinst_DATA) $(noinst_SCRIPTS)

clean-local:
	-rm -f *.map *.log 2hd.img es.img

AM_LDFLAGS = -Wl,--section-start,".init"=0x81000000,-static,--omagic,--cref,-Map,$@.map

endif ES

iso9660_SOURCES = iso9660.cpp vdisk.h

iso9660_create_SOURCES = iso9660_create.cpp vdisk.h

iso9660_attribute_SOURCES = iso9660_attribute.cpp vdisk.h

iso9660_readwrite_SOURCES = iso9660_readwrite.cpp vdisk.h

iso9660_read_SOURCES = iso9660_read.cpp vdisk.h

iso9660_maxLevel_SOURCES = iso9660_maxLevel.cpp vdisk.h

iso9660_getStream_SOURCES = iso9660_getStream.cpp vdisk.h

iso9660_object_SOURCES = iso9660_object.cpp vdisk.h

iso9660_remove_SOURCES = iso9660_remove.cpp vdisk.h

iso9660_size_SOURCES = iso9660_size.cpp vdisk.h

iso9660_time_SOURCES = iso9660_time.cpp vdisk.h

iso9660_rename_SOURCES = iso9660_rename.cpp vdisk.h

isotest.iso: isotest.iso.gz
	gunzip -c $< > $@

fat16_5MB.img : fat16_5MB.img.gz
	gunzip -c $< > $@

AM_CPPFLAGS = \
	-I$(srcdir)/.. \
	-I- \
	-I../../../../include \
	-I$(srcdir)/../../../../include

EXTRA_DIST = isotest.iso.gz fat16_5MB.img.gz makeScript testLoader
