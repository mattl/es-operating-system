SUBDIRS = . testsuite

lib_LIBRARIES = libesfat.a

if ES
AM_CCASFLAGS = -DIA32 -DNINTENDO_ES
AM_CFLAGS = -DIA32 -DNINTENDO_ES
AM_CXXFLAGS = -DIA32 -DNINTENDO_ES
endif

SUFFIXES = .bin
vpath %.bin = ../../bootsect

.bin.c:
	echo "unsigned char" $(*F) "[] = {" > $@;
	od -A n -t x1 -v -w8 $< | sed -e 's/^ */0x/' -e 's/ /, 0x/g' -e 's/$$/,/' >> $@;
	echo "};" >> $@

s_source_files =

c_source_files =

cpp_source_files = \
	fatBinding.cpp \
	fatCheck.cpp \
	fatCluster.cpp \
	fatContext.cpp \
	fatDirectory.cpp \
	fatFile.cpp \
	fatFileSystem.cpp \
	fatFormat.cpp \
	fatIterator.cpp \
	fatStream.cpp \
	fatUtf.cpp

header_files = \
	fat.h \
	fatStream.h

libesfat_a_SOURCES = $(s_source_files) $(c_source_files) $(cpp_source_files) $(header_files)

nodist_libesfat_a_SOURCES = \
	bootfat12.c \
	bootfat16.c \
	bootfat32.c

INCLUDES = \
	-I $(srcdir) \
	-I- \
	-I ../../../include \
	-I $(srcdir)/../../../include
