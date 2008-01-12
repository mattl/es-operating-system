LDADD = ../libesnet.a ../../kernel/libeskernel.a ../../libes++/libessup++.a

SUFFIXES = .img

%.img : %.elf
	$(host_alias)-objcopy -O binary $< $@

% :: %.elf
	$(srcdir)/makeScript "`pwd`" $@

AM_CPPFLAGS = \
	-I $(srcdir)/../include \
	-I- \
	-I ../../../include \
	-I $(srcdir)/../../../include

if POSIX

TESTS = inet4 tcp config anon unreach mcast frag

noinst_PROGRAMS = $(TESTS)

LDADD += -lpthread -lrt

endif POSIX

if ES

TESTS = config

noinst_SCRIPTS = $(TESTS)

noinst_PROGRAMS = $(TESTS)

noinst_DATA = config.img

CLEANFILES = $(noinst_DATA) $(noinst_SCRIPTS)

clean-local:
	-rm -f *.map *.log 2hd.img es.img

AM_LDFLAGS = -Wl,--section-start,".init"=0x81000000,-static,--omagic,--cref,-Map,$@.map

endif ES

anon_SOURCES = anon.cpp

frag_SOURCES = frag.cpp

inet4_SOURCES = inet4.cpp

mcast_SOURCES = mcast.cpp

tcp_SOURCES = tcp.cpp

config_SOURCES = config.cpp

unreach_SOURCES = unreach.cpp

EXTRA_DIST = makeScript
