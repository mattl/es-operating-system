SUBDIRS = . testsuite

lib_LIBRARIES = libesiso9660.a

if ES
AM_CCASFLAGS = -DIA32 -DNINTENDO_ES
AM_CFLAGS = -DIA32 -DNINTENDO_ES
AM_CXXFLAGS = -DIA32 -DNINTENDO_ES
endif

s_source_files =

c_source_files =

cpp_source_files = \
	iso9660Ascii.cpp \
	iso9660Binding.cpp \
	iso9660Context.cpp \
	iso9660File.cpp \
	iso9660FileSystem.cpp \
	iso9660Iterator.cpp \
	iso9660Stream.cpp \
	iso9660Ucs2.cpp

header_files = \
	iso9660.h \
	iso9660Stream.h

libesiso9660_a_SOURCES = $(s_source_files) $(c_source_files) $(cpp_source_files) $(header_files)

INCLUDES = \
	-I$(srcdir) \
	-I- \
	-I../../../include \
	-I$(srcdir)/../../../include
