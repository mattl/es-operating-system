if ES
AM_LDFLAGS = -v -static -Wl,--no-omagic,-Map,$@.map,--cref -L$(prefix)/lib
LDADD = ../libes++/libes++.a -lcairo -lfontconfig -lexpat -lfreetype -lm
else 
if APPLE
LDADD = ../libes++/libes++.a ../kernel/libeskernel.a ../libes++/libes++.a
else
LDADD = ../libes++/libes++.a ../kernel/libeskernel.a ../libes++/libes++.a -lrt -lX11
endif
endif

AM_CPPFLAGS = \
	-I- \
	-I ../../include \
	-I $(srcdir)/../../include

TESTS = client server

noinst_PROGRAMS = $(TESTS)

client_SOURCES = client.cpp

server_SOURCES = server.cpp

TESTS : ../libes++/libes++.a ../kernel/libeskernel.a
