SUBDIRS = . testsuite

lib_LIBRARIES = libeskernel.a

SUFFIXES = .ent .c .bin
vpath %.ent = ../../include

%.c : %.ent
	mkdir -p $(*D);
	printf 'unsigned char %s' $(*F) > $@;
	echo "Info[] = {" >> $@;
	hexdump -v -e '1/1 "0x%02x,"' $< >> $@;
	echo "};" >> $@

vpath %.bin = ../bootsect $(srcdir)/../bootsect

.bin.c:
	echo "unsigned char" $(*F) "[] = {" > $@;
	hexdump -v -e '1/1 "0x%02x,"' $< >> $@;
	echo "};" >> $@

AM_CPPFLAGS = \
	-I../../include \
	-I$(srcdir)/../../include \
	-iquote $(srcdir)/include

#
# default interface information
#
nodist_libeskernel_a_SOURCES = \
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
	
#
# port
#

c_source_files =

cpp_source_files = \
	port/elf.cpp \
	port/init.cpp \
	port/interface.cpp

# memory management
cpp_source_files += \
	port/arena.cpp \
	port/cache.cpp \
	port/cacheFactory.cpp \
	port/heap.cpp \
	port/line.cpp \
	port/loopback.cpp \
	port/page.cpp \
	port/pageSet.cpp \
	port/pageTable.cpp \
	port/stream.cpp \
	port/zero.cpp

# The filenames of the header files not intalled but used by the kernel
header_files = \
	include/alarm.h \
	include/arena.h \
	include/cache.h \
	include/elfFile.h \
	include/heap.h \
	include/interfaceStore.h \
	include/line.h \
	include/loopback.h \
	include/partition.h \
	include/process.h \
	include/spinlock.h \
	include/thread.h \
	include/zero.h

# extra source files
cpp_source_files += \
	port/alarm.cpp \
	port/partitionContext.cpp \
	port/partitionStream.cpp \
	port/partitionIterator.cpp

# 	port/zero.cpp

# extra header files
header_files += \
	include/ata.h \
	include/uart.h

# 	include/space.h

nodist_libeskernel_a_SOURCES += mbr.c

#
# pc
#

if ES

if I386
AM_CPPFLAGS += -iquote $(srcdir)/include/i386 -fnon-call-exceptions
endif I386

cpp_source_files += \
	port/lock.cpp \
	port/malloc.cpp \
	port/map.cpp \
	port/mmu.cpp \
	port/monitor.cpp \
	port/process.cpp \
	port/rendezvous.cpp \
	port/sched.cpp \
	port/spinlock.cpp \
	port/swap.cpp \
	port/syscall.cpp \
	port/thread.cpp \
	port/upcall.cpp

# c_source_files += \
#

cpp_source_files += \
	pc/8042.cpp \
	pc/8237a.cpp \
	pc/8254.cpp \
	pc/8259.cpp \
	pc/apic.cpp \
	pc/ataController.cpp \
	pc/ataDevice.cpp \
	pc/ataPacket.cpp \
	pc/cga.cpp \
	pc/fdc.cpp \
	pc/fdd.cpp \
	pc/gdbstub.cpp \
	pc/mps.cpp \
	pc/pci.cpp \
	pc/pc_core.cpp \
	pc/pc_dateTime.cpp \
	pc/pc_init.cpp \
	pc/pc_thread.cpp \
	pc/rtc.cpp \
	pc/sb16.cpp \
	pc/es1370.cpp \
	pc/uart.cpp \
	pc/vesa.cpp \
	pc/xreg.cpp \
	pc/dp8390d.cpp

# cpp_source_files += \
#	pc/acpios.cpp
#	pc/dc21140.cpp
#	pc/elfloader.cpp
#	pc/pci.cpp

s_source_files = \
	pc/label.S

header_files += \
	include/i386/8042.h \
	include/i386/8237a.h \
	include/i386/8254.h \
	include/i386/8259.h \
	include/i386/apic.h \
	include/i386/ataController.h \
	include/i386/cga.h \
	include/i386/core.h \
	include/i386/cpu.h \
	include/i386/fdc.h \
	include/i386/io.h \
	include/i386/label.h \
	include/i386/mps.h \
	include/i386/pci.h \
	include/i386/rtc.h \
	include/i386/sb16.h \
	include/i386/es1370.h \
	include/i386/vesa.h \
	include/i386/dp8390d.h

# header_files += \
#	pc/dc21140.h
#	pc/elfloader.h
#	pc/pci.h

endif ES

#
# posix
#

if POSIX

cpp_source_files += \
	posix/posix_thread.cpp \
	posix/posix_monitor.cpp \
	posix/posix_core.cpp \
	posix/posix_dateTime.cpp \
	posix/posix_init.cpp \
	posix/posix_ethernet.cpp

header_files += \
	include/posix/core.h \
	include/posix/tap.h

endif POSIX

libeskernel_a_SOURCES = $(s_source_files) $(c_source_files) $(cpp_source_files) $(header_files)

clean-local:
	-rm -rf base device naming util

if POSIX
AM_CPPFLAGS += -iquote $(srcdir)/include/posix
endif POSIX
