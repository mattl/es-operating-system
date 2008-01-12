SUBDIRS = . testsuite

lib_LIBRARIES = libessup++.a libes++.a

AM_CPPFLAGS = \
	-I $(srcdir)/include \
	-I- \
	-I../../include \
	-I$(srcdir)/../../include

c_source_files = \
	src/md5.c \
	src/rand48.c \
	src/string.c \
	src/utf.c

cpp_source_files = \
	src/context.cpp \
	src/dateTime.cpp \
	src/inet.cpp \
	src/throw.cpp \
	src/formatter.cpp \
	src/reflect.cpp \
	src/ring.cpp

header_files = \
	include/core.h

libessup___a_SOURCES = $(c_source_files) $(cpp_source_files) $(header_files)
libes___a_SOURCES = $(libessup___a_SOURCES) src/report.cpp

if ES

libes___a_SOURCES += src/system.cpp

endif ES

if POSIX

libes___a_SOURCES += src/posix_system.cpp

endif POSIX
