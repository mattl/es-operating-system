SUBDIRS = . testsuite

SUFFIXES = .h .idl .ird

%.h %.ird : %.idl
	esidl -I $(srcdir)/../include $<

%.c : %.ird
	mkdir -p $(*D);
	echo -n "unsigned char" $(*F) > $@;
	echo "Info[] = {" >> $@;
	od -A n -t x1 -v -w8 $< | sed -e 's/^ */0x/' -e 's/ /, 0x/g' -e 's/$$/,/' >> $@;
	echo "};" >> $@
	echo -n "unsigned" $(*F) >> $@;
	echo -n "InfoSize = sizeof" $(*F) >> $@;
	echo "Info;" >> $@;

noinst_PROGRAMS = main hello server client binder binderClient location locationClient \
                  eventManager eventManagerClient console consoleClient upcallTest upcallTestClient

AM_LDFLAGS = -v -static -Wl,--no-omagic,-Map,$@.map,--cref
AM_CPPFLAGS = -I- \
	-I../include \
	-I$(srcdir)/../include \
	-I$(prefix)/include \
	-I$(prefix)/include/freetype2

LDADD = ../os/libes++/libes++.a $(prefix)/lib/libfreetype.a

hello_SOURCES = hello.cpp

main_SOURCES = main.cpp

server_SOURCES = server.cpp

client_SOURCES = client.cpp

binder_SOURCES = binder.cpp binder.h

binderClient_SOURCES = binderClient.cpp binder.h

location_SOURCES = location.cpp location.h ILocation.idl

locationClient_SOURCES = locationClient.cpp location.h ILocation.idl

nodist_location_SOURCES = ILocation.c

BUILT_SOURCES = ILocation.h ILocation.ird

eventManager_SOURCES = eventManager.cpp eventManager.h IEventQueue.idl

eventManagerClient_SOURCES = eventManagerClient.cpp eventManager.h IEventQueue.idl

nodist_eventManager_SOURCES = IEventQueue.c

BUILT_SOURCES += IEventQueue.h IEventQueue.ird

console_SOURCES = console.cpp

consoleClient_SOURCES = consoleClient.cpp

upcallTest_SOURCES = upcallTest.cpp

upcallTestClient_SOURCES = upcallTestClient.cpp

clean-local:
	-rm -f $(BUILT_SOURCES)
