if ES
AM_LDFLAGS = -v -static -Wl,--no-omagic,-Map,$@.map,--cref -L$(prefix)/lib
LDADD = ../libes++/libes++.a -lcairo -lfontconfig -lexpat -lfreetype -lm
else 
if APPLE
LDADD = ../libes++/libes++.a ../kernel/libeskernel.a ../libes++/libes++.a -lX11 -lcairo
else
LDADD = ../libes++/libes++.a ../kernel/libeskernel.a ../libes++/libes++.a -lrt -lX11 -lcairo
endif
endif

AM_CPPFLAGS = \
	-I- \
	-I ../../include \
	-I $(srcdir)/../../include \
	-I/usr/include/cairo

TESTS = client server video cairo

noinst_PROGRAMS = $(TESTS)

client_SOURCES = client.cpp

server_SOURCES = server.cpp

video_SOURCES = video.cpp

cairo_SOURCES = cairo.cpp

TESTS : ../libes++/libes++.a ../kernel/libeskernel.a
