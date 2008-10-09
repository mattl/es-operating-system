SUBDIRS = . testsuite

SUFFIXES = .ent

vpath %.ent = ../../include/es

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

rpc_source_files = \
	src/interfaceStore.cpp
	
if I386
rpc_source_files += src/variant386.S
endif I386

if X86_64
rpc_source_files += src/variantx64.S
endif X86_64

header_files = \
	include/core.h \
	include/posix_system.h \
	include/posix_video.h

libessup___a_SOURCES = $(c_source_files) $(cpp_source_files) $(rpc_source_files) $(header_files)

#
# default interface descriptions
#
nodist_libessup___a_SOURCES = \
	base/IAlarm.c \
	base/ICache.c \
	base/ICallback.c \
	base/IClassFactory.c \
	base/IClassStore.c \
	base/IFile.c \
	base/IInterface.c \
	base/IInterfaceStore.c \
	base/IMonitor.c \
	base/IPageable.c \
	base/IPageSet.c \
	base/IProcess.c \
	base/IRuntime.c \
	base/ISelectable.c \
	base/IService.c \
	base/IStream.c \
	base/IThread.c \
	device/IAudioFormat.c \
	device/IBeep.c \
	device/ICursor.c \
	device/IDevice.c \
	device/IDiskManagement.c \
	device/IDmac.c \
	device/IFileSystem.c \
	device/INetworkInterface.c \
	device/IPic.c \
	device/IRemovableMedia.c \
	device/IRtc.c \
	device/IPartition.c \
	naming/IBinding.c \
	naming/IContext.c \
	net/IInternetAddress.c \
	net/IInternetConfig.c \
	net/IResolver.c \
	net/ISocket.c \
	util/IIterator.c \
	util/ISet.c \
	util/ICanvasRenderingContext2D.c \
	util/IOrderedMap.c

libes___a_SOURCES = $(libessup___a_SOURCES) $(nodist_libessup___a_SOURCES) src/report.cpp

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

