if POSIX
SUBDIRS = . testsuite
endif POSIX

BUILT_SOURCES = lexer.cc parser.cc parser.h

esjs_SOURCES = \
	src/lexer.ll src/parser.yy src/esjs.cpp src/value.cpp \
	src/array.cpp \
	src/boolean.cpp \
	src/date.cpp \
	src/function.cpp \
	src/global.cpp \
	src/interface.cpp \
	src/math.cpp \
	src/number.cpp \
	src/object.cpp \
	src/regexp.cpp \
	src/string.cpp \
	include/esjs.h include/value.h include/interface.h include/smartptr.h

bin_PROGRAMS = esjs 

if POSIX

if APPLE
LDADD = ../os/libes++/libes++.a ../os/fs/fat/libesfat.a ../os/fs/iso9660/libesiso9660.a ../os/kernel/libeskernel.a ../os/libes++/libes++.a -lpcreposix -lpcre -lglut -lGLU -lGL -lXmu -lXi -lXext -lX11 -lcairo -lpixman-1
else
LDADD = ../os/libes++/libes++.a ../os/fs/fat/libesfat.a ../os/fs/iso9660/libesiso9660.a ../os/kernel/libeskernel.a ../os/libes++/libes++.a -lpcreposix -lpcre -lrt -lglut -lGLU -lGL -lXmu -lXi -lXext -lX11 -lcairo -lpixman-1
endif

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
	-I../include \
	-I$(prefix)/include

clean-local:
	-rm -f $(BUILT_SOURCES) parser.output

dist-hook:
	-rm -f $(distdir)/lexer.cc $(distdir)/parser.cc $(distdir)/parser.h

EXTRA_DIST = \
	scripts/cat.js		\
	scripts/cd.js		\
	scripts/clear.js	\
	scripts/date.js		\
	scripts/echo.js		\
	scripts/edit.js		\
	scripts/figure.js	\
	scripts/gradient.js	\
	scripts/kanji.js	\
	scripts/ls.js		\
	scripts/pi.js		\
	scripts/rm.js		\
	scripts/shell.js

