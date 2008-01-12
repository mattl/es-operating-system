SUBDIRS = . testsuite

noinst_PROGRAMS = main server client binder binderClient

AM_LDFLAGS = -v -static -Wl,--no-omagic,-Map,$@.map,--cref
AM_CPPFLAGS = -I- \
	-I../include \
	-I$(srcdir)/../include

LDADD = ../os/libes++/libes++.a

main_SOURCES = main.cpp

server_SOURCES = server.cpp

client_SOURCES = client.cpp

binder_SOURCES = binder.cpp binder.h

binderClient_SOURCES = binderClient.cpp binder.h
