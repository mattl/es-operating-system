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

TESTS = inet4 tcp config anon unreach mcast frag timeout dhcp dns

noinst_PROGRAMS = $(TESTS)

LDADD += -lpthread -lrt

endif POSIX

if ES

TESTS = config dhcp

noinst_SCRIPTS = $(TESTS)

noinst_PROGRAMS = $(TESTS)

noinst_DATA = config.img dhcp.img

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

timeout_SOURCES = timeout.cpp

config_SOURCES = config.cpp

dhcp_SOURCES = dhcp.cpp

dns_SOURCES = dns.cpp

unreach_SOURCES = unreach.cpp

EXTRA_DIST = makeScript
