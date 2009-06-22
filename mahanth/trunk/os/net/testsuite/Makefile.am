LDADD = ../libesnet.a ../../kernel/libeskernel.a ../../libes++/libessup++.a

SUFFIXES = .img

%.img : %.elf
	$(host_alias)-objcopy -O binary $< $@
	$(srcdir)/makeScript "`pwd`" $(*F)

AM_CPPFLAGS = \
	-I $(srcdir)/../include \
	-I- \
	-I ../../../include \
	-I $(srcdir)/../../../include

if POSIX

TESTS = inet4 tcp tcp1 tcp2 config anon unreach mcast frag timeout dhcp dns \
	udpEchoClient udpEchoServer tcpdiscardClient tcpdiscardServer tcpTimeout

noinst_PROGRAMS = $(TESTS)

if APPLE
LDADD += -lpthread
else
LDADD += -lpthread -lrt
endif

endif POSIX

if ES

TESTS_ENVIRONMENT = $(srcdir)/testLoader

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

tcp1_SOURCES = tcp1.cpp

tcp2_SOURCES = tcp2.cpp

timeout_SOURCES = timeout.cpp

config_SOURCES = config.cpp

dhcp_SOURCES = dhcp.cpp

dns_SOURCES = dns.cpp

unreach_SOURCES = unreach.cpp

udpEchoClient_SOURCES = udpEchoClient.cpp

udpEchoServer_SOURCES = udpEchoServer.cpp

tcpdiscardClient_SOURCES = tcpdiscardClient.cpp

tcpdiscardServer_SOURCES = tcpdiscardServer.cpp

tcpTimeout_SOURCES = tcpTimeout.cpp

EXTRA_DIST = makeScript testLoader
