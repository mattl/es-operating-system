if ES

AM_CPPFLAGS = -I- \
	-I../../include \
	-I$(srcdir)/../../include \
	-I$(prefix)/include \
	-I$(prefix)/include/freetype2 \
	-I$(prefix)/include/cairo \
	-I$(prefix)/include/pixman-1 

AM_LDFLAGS = -v -static -Wl,--no-omagic,-Map,$@.map,--cref -L$(prefix)/lib

LDADD = ../libes++/libes++.a -lcairo -lfontconfig -lexpat -lfreetype -lm -lpixman-1

else

AM_CPPFLAGS = \
	-I- \
	-I ../../include \
	-I $(srcdir)/../../include \
	-I/usr/include/cairo \
	-I/usr/include/pixman-1

if APPLE
LDADD = ../libes++/libes++.a ../kernel/libeskernel.a ../libes++/libes++.a -lglut -lGLU -lGL -lXmu -lXi -lXext -lX11 -lcairo -lpixman-1 
else
LDADD = ../libes++/libes++.a ../kernel/libeskernel.a ../libes++/libes++.a -lrt -lglut -lGLU -lGL -lXmu -lXi -lXext -lX11 -lcairo -lpixman-1
endif

endif

TESTS = client server video cairo canvas testInterfaceStore

noinst_PROGRAMS = $(TESTS)

client_SOURCES = client.cpp

server_SOURCES = server.cpp

video_SOURCES = video.cpp

cairo_SOURCES = cairo.cpp

canvas_SOURCES = canvas.cpp canvas2d.cpp canvas2d.h

testInterfaceStore_SOURCES = testInterfaceStore.cpp

TESTS : ../libes++/libes++.a ../kernel/libeskernel.a

