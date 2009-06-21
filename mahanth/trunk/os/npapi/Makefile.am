lib_LTLIBRARIES = libesnpapi.la

AM_CPPFLAGS = \
	-fPIC -fno-rtti -fno-exceptions \
	-Wall -Wconversion -Wpointer-arith -Woverloaded-virtual -Wsynth -Wno-ctor-dtor-privacy -Wno-non-virtual-dtor -Wcast-align -Wno-long-long \
	-pedantic -fno-strict-aliasing -fshort-wchar -pthread -pipe \
	-I$(srcdir)/include \
	-I$(GECKO_SDK)/include \
	-I- \
	-I../../include \
	-I$(srcdir)/../../include \
	-DXP_UNIX

LDFLAGS = -lpthread -ldl -lm -lesrpc

cpp_source_files = \
	src/np_entry.cpp \
	src/npn_gate.cpp \
	src/npp_gate.cpp \
	src/bridge.cpp \
	src/launcher.cpp \
	src/exportedObject.cpp \
	src/importedObject.cpp \
	src/thread.cpp \
	src/debug.cpp

header_files = \
	include/bridge.h \
	include/monitor.h \
	include/exportedObject.h \
	include/importedObject.h

libesnpapi_la_SOURCES = $(cpp_source_files) $(header_files)

