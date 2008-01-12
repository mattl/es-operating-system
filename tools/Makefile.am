LDADD = ../os/fs/fat/libesfat.a ../os/kernel/libeskernel.a ../os/libes++/libes++.a -lrt

bin_PROGRAMS = trunc bm vformat vcopy vlist guidgen

trunc_SOURCES = trunc.c

bm_SOURCES = bm.c

vformat_SOURCES = vformat.cpp vdisk.h

vcopy_SOURCES = vcopy.cpp vdisk.h

vlist_SOURCES = vlist.cpp vdisk.h

guidgen_SOURCES = guidgen.cpp

AM_CPPFLAGS = -I- -I../include -I$(srcdir)/../include
