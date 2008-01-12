lib_LIBRARIES = libes++.a libessup++.a

AM_CPPFLAGS = \
	-I../../include \
	-I$(srcdir)/../../include \
	-iquote $(srcdir)/include

c_source_files = \
	src/md5.c \
	src/string.c \
	src/utf.c

cpp_source_files = \
	src/dateTime.cpp \
	src/throw.cpp \
	src/formatter.cpp \
	src/ring.cpp

header_files =

libessup___a_SOURCES = $(c_source_files) $(cpp_source_files) $(header_files)

libes___a_SOURCES = $(libessup___a_SOURCES) src/system.cpp
