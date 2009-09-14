SUBDIRS = . testsuite posix

lib_LIBRARIES = libesnet.a

AM_CPPFLAGS = \
	-I../../include \
	-I$(srcdir)/../../include \
	-iquote $(srcdir)/include

c_source_files =

cpp_source_files = \
	src/arp.cpp \
	src/conduit.cpp \
	src/datagram.cpp \
	src/dhcp.cpp \
	src/dix.cpp \
	src/icmp4.cpp \
	src/igmp.cpp \
	src/inet4address.cpp \
	src/inet4reass.cpp \
	src/inet4.cpp \
	src/inet6address.cpp \
	src/inet6.cpp \
	src/inetConfig.cpp \
	src/inet.cpp \
	src/resolver.cpp \
	src/socket.cpp \
	src/stream.cpp \
	src/streamInput.cpp \
	src/streamOutput.cpp \
	src/streamScoreboard.cpp \
	src/streamTimer.cpp \
	src/tcp.cpp \
	src/udp.cpp

header_files = \
	include/address.h \
	include/arp.h \
	include/conduit.h \
	include/datagram.h \
	include/dix.h \
	include/icmp4.h \
	include/igmp.h \
	include/inet4address.h \
	include/inet4reass.h \
	include/inet4.h \
	include/inet6address.h \
	include/inet6.h \
	include/inetConfig.h \
	include/inet.h \
	include/interface.h \
	include/loopback.h \
	include/resolver.h \
	include/socket.h \
	include/stream.h \
	include/tcp.h \
	include/udp.h \
	include/visualizer.h

libesnet_a_SOURCES = $(c_source_files) $(cpp_source_files) $(header_files)
