SUBDIRS = . testsuite

SUFFIXES = .ent

vpath %.ent = ../../include

%.c : %.ent
	mkdir -p $(*D);
	printf 'unsigned char %s' $(*F) > $@;
	echo "Info[] = {" >> $@;
	hexdump -v -e '1/1 "0x%02x,"' $< >> $@;
	echo "};" >> $@

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
libessup___a_SOURCES += src/variant386.S
endif I386

if X86_64
libessup___a_SOURCES += src/variantx64.S
endif X86_64

#
# default interface descriptions
#
nodist_libessup___a_SOURCES = \
	es/object.c \
	es/base/IAlarm.c \
	es/base/ICache.c \
	es/base/ICallback.c \
	es/base/IFile.c \
	es/base/IInterfaceStore.c \
	es/base/IMonitor.c \
	es/base/IPageable.c \
	es/base/IPageSet.c \
	es/base/IProcess.c \
	es/base/IRuntime.c \
	es/base/ISelectable.c \
	es/base/IService.c \
	es/base/IStream.c \
	es/base/IThread.c \
	es/device/IAudioFormat.c \
	es/device/IBeep.c \
	es/device/ICursor.c \
	es/device/IDevice.c \
	es/device/IDiskManagement.c \
	es/device/IDmac.c \
	es/device/IFatFileSystem.c \
	es/device/IFileSystem.c \
	es/device/IIso9660FileSystem.c \
	es/device/INetworkInterface.c \
	es/device/IPic.c \
	es/device/IRemovableMedia.c \
	es/device/IRtc.c \
	es/device/IPartition.c \
	es/naming/IBinding.c \
	es/naming/IContext.c \
	es/net/IInternetAddress.c \
	es/net/IInternetConfig.c \
	es/net/IResolver.c \
	es/net/ISocket.c \
	es/util/IIterator.c \
	es/util/ISet.c \
	es/util/IOrderedMap.c \
	w3c/css.c \
	w3c/cssomview.c \
	w3c/dom.c \
	w3c/events.c \
	w3c/html5.c \
	w3c/ls.c \
	w3c/ranges.c \
	w3c/smil.c \
	w3c/stylesheets.c \
	w3c/svg.c \
	w3c/traversal.c \
	w3c/validation.c \
	w3c/views.c

libes___a_SOURCES = $(libessup___a_SOURCES) $(nodist_libessup___a_SOURCES) \
	src/interfaceStore.cpp \
	src/report.cpp

if ES

libes___a_SOURCES += src/system.cpp

endif ES

if POSIX

# libesrpc is built for shared libraries
lib_LIBRARIES += libesrpc.a

AM_CPPFLAGS += -I/usr/include/GL

libes___a_SOURCES += src/posix_system.cpp src/posix_video.cpp src/rpc.cpp

libesrpc_a_SOURCES = $(rpc_source_files) $(nodist_libessup___a_SOURCES) src/rpc.cpp

# -fno-exceptions
libesrpc_a_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-fPIC -fno-rtti \
	-Wall -Wconversion -Wpointer-arith -Woverloaded-virtual -Wsynth -Wno-ctor-dtor-privacy -Wno-non-virtual-dtor -Wcast-align -Wno-long-long \
	-pedantic -fno-strict-aliasing -fshort-wchar -pthread -pipe

endif POSIX

