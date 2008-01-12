noinst_PROGRAMS = main

AM_LDFLAGS = -v -static -Wl,--no-omagic,-Map,$@.map,--cref
AM_CPPFLAGS = -I- \
	-I../include \
	-I$(srcdir)/../include

LDADD = ../os/libes++/libes++.a

main_SOURCES = main.cpp
