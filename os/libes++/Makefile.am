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
	src/apply386.S \
	src/md5.c \
	src/rand48.c \
	src/string.c \
	src/utf.c

cpp_source_files = \
	src/apply.cpp \
	src/color.cpp \
	src/context.cpp \
	src/dateTime.cpp \
	src/dump.cpp \
	src/inet.cpp \
	src/interfaceStore.cpp \
	src/throw.cpp \
	src/formatter.cpp \
	src/ring.cpp

header_files = \
	include/core.h

libessup___a_SOURCES = $(c_source_files) $(cpp_source_files) $(header_files)

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
	util/ICanvasRenderingContext2D.c

libes___a_SOURCES = $(libessup___a_SOURCES) $(nodist_libessup___a_SOURCES) src/report.cpp

if ES

libes___a_SOURCES += src/system.cpp

endif ES

if POSIX

libes___a_SOURCES += src/posix_system.cpp

endif POSIX
