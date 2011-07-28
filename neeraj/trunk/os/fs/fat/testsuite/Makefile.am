LDADD = ../libesfat.a ../../../kernel/libeskernel.a ../../../libes++/libessup++.a

SUFFIXES = .img .gz 

%.img : %.elf
	$(host_alias)-objcopy -O binary $< $@
	$(srcdir)/makeScript "`pwd`" $(*F)

if POSIX

TESTS = fat replace \
	fat_createdir fat_createfile fat_readwrite fat_attribute fat_size \
	fat_getStream fat_time fat_removedir fat_object fat_writemax \
	fat16 fat16_replace \
	fat16_createdir fat16_createfile fat16_readwrite fat16_attribute fat16_size \
	fat16_getStream fat16_time fat16_removedir fat16_object fat16_writemax \
	fat32_test fat32_replace \
	fat32_createdir fat32_createfile fat32_readwrite fat32_attribute fat32_size \
	fat32_getStream fat32_time fat32_removedir fat32_object fat32_writemax \
	fileSystem \
	fat_createmax fat16_createmax fat32_createmax

noinst_PROGRAMS = $(TESTS)

noinst_DATA = 2hd.img fat16_5MB.img fat32.img

CLEANFILES = 2hd.img fat16_5MB.img fat32.img

if APPLE
LDADD += -lpthread
else
LDADD += -lpthread -lrt
endif

endif POSIX

if ES

TESTS_ENVIRONMENT = $(srcdir)/testLoader

TESTS = fat16 fat16_replace \
	fat16_createdir fat16_createfile fat16_readwrite fat16_attribute fat16_size \
	fat16_getStream fat16_time fat16_removedir fat16_object fat16_writemax \
	fat replace \
	fat_createdir fat_createfile fat_readwrite fat_attribute fat_size \
	fat_getStream fat_time fat_removedir fat_object fat_writemax \
	fat32_test fat32_replace \
	fat32_createdir fat32_createfile fat32_readwrite fat32_attribute fat32_size \
	fat32_getStream fat32_time fat32_removedir fat32_object fat32_writemax \
	fat_createmax fat16_createmax fat32_createmax

noinst_SCRIPTS = $(TESTS)

noinst_PROGRAMS = $(noinst_SCRIPTS)

noinst_DATA = 2hd.img fat16_5MB.img fat32.img \
	fat16.img fat16_replace.img \
	fat16_createdir.img fat16_createfile.img fat16_readwrite.img fat16_attribute.img fat16_size.img \
	fat16_getStream.img fat16_time.img fat16_removedir.img fat16_object.img fat16_writemax.img \
	fat.img replace.img \
	fat_createdir.img fat_createfile.img fat_readwrite.img fat_attribute.img fat_size.img \
	fat_getStream.img fat_time.img fat_removedir.img fat_object.img fat_writemax.img \
	fat32_test.img fat32_replace.img \
	fat32_createdir.img fat32_createfile.img fat32_readwrite.img fat32_attribute.img fat32_size.img \
	fat32_getStream.img fat32_time.img fat32_removedir.img fat32_object.img fat32_writemax.img \
	fat_createmax.img fat16_createmax.img fat32_createmax.img

CLEANFILES = $(noinst_DATA) $(noinst_SCRIPTS)

clean-local:
	-rm -f *.map *.log

AM_LDFLAGS = -Wl,--section-start,".init"=0x81000000,-static,--omagic,--cref,-Map,$@.map

endif ES

fat_SOURCES = fat.cpp vdisk.h

replace_SOURCES = replace.cpp vdisk.h

fat_createdir_SOURCES = fat_createdir.cpp vdisk.h

fat_createfile_SOURCES = fat_createfile.cpp vdisk.h

fat_readwrite_SOURCES = fat_readwrite.cpp vdisk.h

fat_attribute_SOURCES = fat_attribute.cpp vdisk.h

fat_size_SOURCES = fat_size.cpp vdisk.h

fat_getStream_SOURCES = fat_getStream.cpp vdisk.h

fat_time_SOURCES = fat_time.cpp vdisk.h

fat_removedir_SOURCES = fat_removedir.cpp vdisk.h

fat_object_SOURCES = fat_object.cpp vdisk.h

fat_createmax_SOURCES = fat_createmax.cpp vdisk.h

fat_writemax_SOURCES = fat_writemax.cpp vdisk.h

fat16_SOURCES = fat16.cpp vdisk.h

fat16_createdir_SOURCES = fat16_createdir.cpp vdisk.h

fat16_createfile_SOURCES = fat16_createfile.cpp vdisk.h

fat16_readwrite_SOURCES = fat16_readwrite.cpp vdisk.h

fat16_attribute_SOURCES = fat16_attribute.cpp vdisk.h

fat16_size_SOURCES = fat16_size.cpp vdisk.h

fat16_getStream_SOURCES = fat16_getStream.cpp vdisk.h

fat16_time_SOURCES = fat16_time.cpp vdisk.h

fat16_removedir_SOURCES = fat16_removedir.cpp vdisk.h

fat16_object_SOURCES = fat16_object.cpp vdisk.h

fat16_createmax_SOURCES = fat16_createmax.cpp vdisk.h

fat16_writemax_SOURCES = fat16_writemax.cpp vdisk.h

fat16_replace_SOURCES = fat16_replace.cpp vdisk.h

fat32_test_SOURCES = fat32_test.cpp vdisk.h

fat32_createdir_SOURCES = fat32_createdir.cpp vdisk.h

fat32_createfile_SOURCES = fat32_createfile.cpp vdisk.h

fat32_readwrite_SOURCES = fat32_readwrite.cpp vdisk.h

fat32_attribute_SOURCES = fat32_attribute.cpp vdisk.h

fat32_size_SOURCES = fat32_size.cpp vdisk.h

fat32_getStream_SOURCES = fat32_getStream.cpp vdisk.h

fat32_time_SOURCES = fat32_time.cpp vdisk.h

fat32_removedir_SOURCES = fat32_removedir.cpp vdisk.h

fat32_object_SOURCES = fat32_object.cpp vdisk.h

fat32_createmax_SOURCES = fat32_createmax.cpp vdisk.h

fat32_writemax_SOURCES = fat32_writemax.cpp vdisk.h

fat32_replace_SOURCES = fat32_replace.cpp vdisk.h

fileSystem_SOURCES = fileSystem.cpp vdisk.h

2hd.img: 2hd.img.gz
	gunzip -c $< > $@

fat16_5MB.img: fat16_5MB.img.gz
	gunzip -c $< > $@

fat32.img: fat32.img.gz
	gunzip -c $< > $@

AM_CPPFLAGS = \
	-I$(srcdir)/.. \
	-I$(srcdir)/../../../kernel/include \
	-I- \
	-I../../../../include \
	-I$(srcdir)/../../../../include

EXTRA_DIST = 2hd.img.gz fat16_5MB.img.gz fat32.img.gz makeScript testLoader
