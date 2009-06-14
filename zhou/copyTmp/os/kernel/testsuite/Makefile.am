LDADD = ../libeskernel.a ../../libes++/libessup++.a

SUFFIXES = .img .gz .iso

%.img : %.elf
	$(host_alias)-objcopy -O binary $< $@
	$(srcdir)/makeScript "`pwd`" $(*F)

AM_CPPFLAGS = \
	-I $(srcdir)/../include \
	-I- \
	-I ../../../include \
	-I $(srcdir)/../../../include

if POSIX

AM_CPPFLAGS += -iquote $(srcdir)/../include/posix

TESTS = handle interlocked exception utf ring \
	heap page cache replace \
	position size write_read write_read2 write read \
	create_release getPageCount invalidate \
	context datetime thread thread_cancel \
	monitor0 monitor1 monitor2 monitor3 pageSet \
	partition partition_error partition_extended partition_iterator \
	partition_bind partition_print \
	loopback ethernet timer

noinst_PROGRAMS = $(TESTS)

noinst_DATA = fat16_32MB.img fat16.img

if APPLE
LDADD += -lpthread -lstdc++
else
LDADD += -lpthread -lstdc++ -lrt
endif

endif POSIX

if ES

if I386
AM_CPPFLAGS += -iquote $(srcdir)/../include/i386
endif I386

TESTS_ENVIRONMENT = $(srcdir)/testLoader

TESTS = handle interlocked exception utf ring fpu \
	context datetime thread thread_cancel thread_smp \
	monitor0 monitor1 monitor2 monitor3 \
	bpi bpi2 bpi3 \
	ata beep 8042 vesa fdc sb16 es1370 cdrom \
	page cache replace \
	position size write_read write_read2 \
	write read create_release getPageCount \
	invalidate \
	pageSet \
	loopback ethernet

noinst_SCRIPTS = $(TESTS)

noinst_PROGRAMS = $(TESTS) fdc_diskChange

noinst_DATA = fat16.img isotest.iso \
	handle.img interlocked.img exception.img utf.img ring.img fpu.img \
	context.img datetime.img thread.img thread_cancel.img thread_smp.img \
	monitor0.img monitor1.img monitor2.img monitor3.img \
	bpi.img bpi2.img bpi3.img \
	ata.img beep.img 8042.img vesa.img fdc.img \
	fdc_diskChange.img sb16.img es1370.img cdrom.img \
	page.img cache.img replace.img \
	position.img size.img write_read.img write_read2.img \
	write.img read.img create_release.img getPageCount.img \
	invalidate.img \
	pageSet.img \
	ethernet.img loopback.img

CLEANFILES = $(noinst_DATA) $(noinst_SCRIPTS)

clean-local:
	-rm -f *.map *.log 2hd.img es.img

AM_LDFLAGS = -Wl,--section-start,".init"=0x81000000,-static,--omagic,--cref,-Map,$@.map

endif ES

handle_SOURCES = handle.cpp

interlocked_SOURCES = interlocked.cpp

exception_SOURCES = exception.cpp

utf_SOURCES = utf.cpp

ring_SOURCES = ring.cpp

fpu_SOURCES = fpu.cpp

heap_SOURCES = heap.cpp

page_SOURCES = page.cpp

cache_SOURCES = cache.cpp memoryStream.h

ata_SOURCES = ata.cpp

beep_SOURCES = beep.cpp

8042_SOURCES = 8042.cpp

vesa_SOURCES = vesa.cpp

fdc_SOURCES = fdc.cpp

fdc_diskChange_SOURCES = fdc_diskChange.cpp

sb16_SOURCES = sb16.cpp wave.cpp

es1370_SOURCES = es1370.cpp wave.cpp

cdrom_SOURCES = cdrom.cpp

pageSet_SOURCES = pageSet.cpp  memoryStream.h

position_SOURCES = position.cpp memoryStream.h

size_SOURCES = size.cpp

write_read_SOURCES =  write_read.cpp memoryStream.h

write_read2_SOURCES =  write_read2.cpp memoryStream.h

write_SOURCES =  write.cpp memoryStream.h

read_SOURCES =  read.cpp memoryStream.h

create_release_SOURCES =  create_release.cpp memoryStream.h

getPageCount_SOURCES = getPageCount.cpp memoryStream.h

invalidate_SOURCES = invalidate.cpp memoryStream.h

replace_SOURCES = replace.cpp memoryStream.h

context_SOURCES = context.cpp

datetime_SOURCES = datetime.cpp

thread_SOURCES = thread.cpp

thread_smp_SOURCES = thread_smp.cpp

thread_cancel_SOURCES = thread_cancel.cpp

monitor0_SOURCES = monitor0.cpp

monitor1_SOURCES = monitor1.cpp

monitor2_SOURCES = monitor2.cpp

monitor3_SOURCES = monitor3.cpp

bpi_SOURCES = bpi.cpp

bpi2_SOURCES = bpi2.cpp

bpi3_SOURCES = bpi3.cpp

partition_SOURCES = partition.cpp vdisk.h

partition_error_SOURCES = partition_error.cpp vdisk.h

partition_extended_SOURCES = partition_extended.cpp vdisk.h

partition_iterator_SOURCES = partition_iterator.cpp vdisk.h

partition_bind_SOURCES = partition_bind.cpp vdisk.h

partition_print_SOURCES = partition_print.cpp vdisk.h

ethernet_SOURCES = ethernet.cpp

loopback_SOURCES = loopback.cpp

if POSIX

timer_SOURCES = timer.cpp

endif POSIX

fat16_32MB.img: fat16_32MB.img.gz
	gunzip -c $< > $@

fat16.img: fat16.img.gz
	gunzip -c $< > $@

isotest.iso: isotest.iso.gz
	gunzip -c $< > $@

EXTRA_DIST = fat16_32MB.img.gz fat16.img.gz isotest.iso.gz makeScript testLoader

