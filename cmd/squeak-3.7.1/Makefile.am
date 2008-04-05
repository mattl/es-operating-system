noinst_PROGRAMS = squeak

AM_LDFLAGS = -static -Wl,--no-omagic,-Map,$@.map,--cref -L$(prefix)/lib
AM_CPPFLAGS = -I$(srcdir)/../../include -I../../include \
	-I$(prefix)/include -DSQUEAK_BUILTIN_PLUGIN -DDISABLE_SECURITY

AM_CFLAGS = -O2

LDADD = -les++

c_source_files = \
	Cross/vm/sq.h \
	Cross/vm/sqNamedPrims.c \
	Cross/vm/sqVirtualMachine.c \
	Cross/vm/sqVirtualMachine.h \
	es/vm/interp.c \
	es/vm/selector.h \
	es/vm/sqConfig.h \
	es/vm/sqNamedPrims.h \
	es/vm/sqPlatformSpecific.h \
	es/vm/intplugins/B2DPlugin/B2DPlugin.c \
	es/vm/intplugins/BitBltPlugin/BitBltPlugin.c \
	es/vm/intplugins/FilePlugin/FilePlugin.c \
	es/vm/intplugins/FilePlugin/FilePlugin.h \
	es/vm/intplugins/FloatArrayPlugin/FloatArrayPlugin.c \
	es/vm/intplugins/LargeIntegers/LargeIntegers.c \
	es/vm/intplugins/Matrix2x3Plugin/Matrix2x3Plugin.c \
	es/vm/intplugins/MiscPrimitivePlugin/MiscPrimitivePlugin.c \
	es/vm/intplugins/SecurityPlugin/SecurityPlugin.c \
	es/vm/intplugins/SecurityPlugin/SecurityPlugin.h \
	es/vm/intplugins/SecurityPlugin/sqEsSecurity.c \
	es/vm/intplugins/SocketPlugin/SocketPlugin.c \
	es/vm/intplugins/SocketPlugin/SocketPlugin.h \
	es/vm/intplugins/SoundGenerationPlugin/SoundGenerationPlugin.c \
	es/vm/intplugins/SoundGenerationPlugin/SoundGenerationPlugin.h \
	es/vm/intplugins/SoundGenerationPlugin/sqOldSoundPrims.c \
	es/vm/intplugins/SoundPlugin/SoundPlugin.c \
	es/vm/intplugins/SoundPlugin/SoundPlugin.h

cpp_source_files = \
	es/vm/sqEs.cpp \
	es/vm/sqEsInput.cpp \
	es/vm/intplugins/FilePlugin/sqEsFile.cpp \
	es/vm/intplugins/FilePlugin/sqEsDirectory.cpp \
	es/vm/intplugins/SocketPlugin/sqEsSocket.cpp \
	es/vm/intplugins/SoundPlugin/sqEsSound.cpp

squeak_SOURCES = $(c_source_files) $(cpp_source_files)

INCLUDES = \
	-I$(srcdir)/Cross/vm \
	-I$(srcdir)/es/vm \
	-I$(srcdir)/es/vm/intplugins/B2DPlugin \
	-I$(srcdir)/es/vm/intplugins/BitBltPlugin \
	-I$(srcdir)/es/vm/intplugins/FilePlugin \
	-I$(srcdir)/es/vm/intplugins/FloatArrayPlugin \
	-I$(srcdir)/es/vm/intplugins/LargeIntegers \
	-I$(srcdir)/es/vm/intplugins/Matrix2x3Plugin \
	-I$(srcdir)/es/vm/intplugins/MiscPrimitivePlugin \
	-I$(srcdir)/es/vm/intplugins/SocketPlugin \
	-I$(srcdir)/es/vm/intplugins/SoundPlugin \
	-I$(srcdir)/es/vm/intplugins/SoundGenerationPlugin \
	-I$(srcdir)/es/vm/intplugins/SecurityPlugin

EXTRA_DIST =\
	LICENSE \
	NOTICE \
	es/plugins.ext \
	es/plugins.int
