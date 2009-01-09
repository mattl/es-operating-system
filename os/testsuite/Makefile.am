if ES

AM_CPPFLAGS = -I- \
	-I../../include \
	-I$(srcdir)/../../include \
	-I$(prefix)/include \
	-I$(prefix)/include/freetype2 \
	-I$(prefix)/include/cairo

AM_LDFLAGS = -v -static -Wl,--no-omagic,-Map,$@.map,--cref -L$(prefix)/lib

LDADD = ../libes++/libes++.a -lcairo -lfontconfig -lexpat -lfreetype -lm

else

AM_CPPFLAGS = \
	-I- \
	-I ../../include \
	-I $(srcdir)/../../include \
	-I/usr/include/cairo

if APPLE
LDADD = ../libes++/libes++.a ../kernel/libeskernel.a ../libes++/libes++.a -lglut -lGLU -lGL -lXmu -lXi -lXext -lX11 -lcairo 
else
LDADD = ../libes++/libes++.a ../kernel/libeskernel.a ../libes++/libes++.a -lrt -lglut -lGLU -lGL -lXmu -lXi -lXext -lX11 -lcairo
endif

endif

TESTS = client server video cairo canvas interfaceStore

noinst_PROGRAMS = $(TESTS)

client_SOURCES = client.cpp

server_SOURCES = server.cpp

video_SOURCES = video.cpp

cairo_SOURCES = cairo.cpp

canvas_SOURCES = canvas.cpp canvas2d.cpp canvas2d.h

interfaceStore_SOURCES = interfaceStore.cpp

TESTS : ../libes++/libes++.a ../kernel/libeskernel.a

