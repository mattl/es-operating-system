if POSIX
SUBDIRS = . testsuite
endif POSIX

SUFFIXES = .ird

vpath %.ird = ../include/es

%.c : %.ird
	mkdir -p $(*D);
	echo -n "unsigned char" $(*F) > $@;
	echo "Info[] = {" >> $@;
	od -A n -t x1 -v -w8 $< | sed -e 's/^ */0x/' -e 's/ /, 0x/g' -e 's/$$/,/' >> $@;
	echo "};" >> $@

# default interface information
nodist_esjs_SOURCES = \
	base/IAlarm.c \
	base/ICache.c \
	base/ICallback.c \
	base/IClassFactory.c \
	base/IClassStore.c \
	base/IFile.c \
	base/IInterface.c \
	base/IInterfaceStore.c \
	base/IMonitor.c \
	base/IPageable.c \
	base/IPageSet.c \
	base/IProcess.c \
	base/IRuntime.c \
	base/ISelectable.c \
	base/IService.c \
	base/IStream.c \
	base/IThread.c \
	device/IAudioFormat.c \
	device/IBeep.c \
	device/ICursor.c \
	device/IDevice.c \
	device/IDiskManagement.c \
	device/IDmac.c \
	device/IFileSystem.c \
	device/INetworkInterface.c \
	device/IPic.c \
	device/IRemovableMedia.c \
	device/IRtc.c \
	device/IPartition.c \
	naming/IBinding.c \
	naming/IContext.c \
	net/IInternetAddress.c \
	net/IInternetConfig.c \
	net/IResolver.c \
	net/ISocket.c \
	util/IIterator.c \
	util/ISet.c

BUILT_SOURCES = lexer.cc parser.cc parser.h

esjs_SOURCES = \
	src/apply.S src/lexer.ll src/parser.yy src/esjs.cpp src/value.cpp \
	src/array.cpp \
	src/boolean.cpp \
	src/date.cpp \
	src/interface.cpp \
	src/math.cpp \
	src/number.cpp \
	src/regexp.cpp \
	src/string.cpp \
	include/esjs.h include/value.h include/interface.h include/smartptr.h

bin_PROGRAMS = esjs 

if POSIX

LDADD = ../os/libes++/libes++.a ../os/kernel/libeskernel.a ../os/libes++/libes++.a -lrt -lpcreposix -lpcre
esjs_CXXFLAGS = -D YYERROR_VERBOSE

valueptr_SOURCES = src/valueptr.cpp
noinst_PROGRAMS = valueptr

endif POSIX

if ES

AM_LDFLAGS = -static -Wl,--no-omagic,-Map,$@.map,--cref -L$(prefix)/lib

LDADD = ../os/libes++/libes++.a -lpcreposix -lpcre

endif ES

AM_YFLAGS = -d -v -t

AM_CPPFLAGS = \
	-I$(srcdir)/include \
	-I$(srcdir)/src \
	-I- \
	-I$(srcdir)/../include \
	-I$(prefix)/include

clean-local:
	-rm -f $(BUILT_SOURCES) parser.output

dist-hook:
	-rm -f $(distdir)/lexer.cc $(distdir)/parser.cc $(distdir)/parser.h

EXTRA_DIST = \
	scripts/cat.js		\
	scripts/cd.js		\
	scripts/echo.js		\
	scripts/edit.js		\
	scripts/ls.js		\
	scripts/shell.js	\
	scripts/date.js
