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
	src/color.cpp \
	src/context.cpp \
	src/dateTime.cpp \
	src/dump.cpp \
	src/inet.cpp \
	src/throw.cpp \
	src/formatter.cpp \
	src/ring.cpp

header_files = \
	include/core.h \
	include/posix_system.h \
	include/posix_video.h

libessup___a_SOURCES = $(c_source_files) $(cpp_source_files) $(header_files)

if I386
libessup___a_SOURCES += src/variant386.cpp
endif I386

if X86_64
libessup___a_SOURCES += src/variantx64.S
endif X86_64

#
# default interface descriptions
#
nodist_libessup___a_SOURCES = \
	interfaceList.cpp

nodist_libes___a_SOURCES = $(nodist_libessup___a_SOURCES)

libes___a_SOURCES = $(libessup___a_SOURCES)  \
	src/interfaceStore.cpp \
	src/report.cpp

interfaceList.cpp : ../../include/interface.list $(srcdir)/interfaceList.awk
	$(srcdir)/interfaceList.awk $< > $@

if ES

libes___a_SOURCES += src/system.cpp

endif ES

if POSIX

# libesrpc is built for shared libraries
lib_LIBRARIES += libesrpc.a

AM_CPPFLAGS += -I/usr/include/GL

libes___a_SOURCES += src/posix_system.cpp src/posix_video.cpp src/rpc.cpp

nodist_libesrpc_a_SOURCES = $(nodist_libessup___a_SOURCES)

libesrpc_a_SOURCES = $(rpc_source_files)  src/rpc.cpp

# -fno-exceptions
libesrpc_a_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-fPIC -fno-rtti \
	-Wconversion -Wpointer-arith -Woverloaded-virtual -Wsynth -Wno-ctor-dtor-privacy -Wno-non-virtual-dtor -Wcast-align -Wno-long-long \
	-fno-strict-aliasing -fshort-wchar -pthread -pipe

endif POSIX

clean-local:
	rm -f interfaceList.cpp

EXTRA_DIST = interfaceList.awk

