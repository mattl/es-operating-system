LDADD = ../libessup++.a

AM_CPPFLAGS = \
	-I $(srcdir)/../include \
	-I- \
	-I ../../../include \
	-I $(srcdir)/../../../include

if POSIX

TESTS = list hashtable

noinst_PROGRAMS = $(TESTS)

list_SOURCES = list.cpp

hashtable_SOURCES = hashtable.cpp

LDADD += -lpthread -lrt

endif POSIX
