if APPLE
LDADD = ../os/fs/fat/libesfat.a ../os/kernel/libeskernel.a ../os/libes++/libessup++.a -lpthread -lstdc++
else
LDADD = ../os/fs/fat/libesfat.a ../os/kernel/libeskernel.a ../os/libes++/libessup++.a -lpthread -lstdc++ -lrt
endif
	
bin_PROGRAMS = trunc bm vformat vcopy vlist guidgen vget vdel

trunc_SOURCES = trunc.c

bm_SOURCES = bm.c

vformat_SOURCES = vformat.cpp vdisk.h

vdel_SOURCES = vdel.cpp vdisk.h

vget_SOURCES = vget.cpp vdisk.h

vcopy_SOURCES = vcopy.cpp vdisk.h

vlist_SOURCES = vlist.cpp vdisk.h

guidgen_SOURCES = guidgen.cpp

AM_CPPFLAGS = -I- \
	-I../include \
	-I$(srcdir)/../include \
	-I$(srcdir)/../os/fs/fat \
	-I$(srcdir)/../os/fs/iso9660

dist_bin_SCRIPTS = idl_scraper.pl

