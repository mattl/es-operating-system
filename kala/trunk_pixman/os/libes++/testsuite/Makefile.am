LDADD = ../libessup++.a

AM_CPPFLAGS = \
	-I $(srcdir)/../include \
	-I- \
	-I ../../../include \
	-I $(srcdir)/../../../include

if POSIX

TESTS = broker collection colorTest list hashtable tree rand smartptr formatter variant

noinst_PROGRAMS = $(TESTS)

broker_SOURCES = broker.cpp

collection_SOURCES = collection.cpp

colorTest_SOURCES = colorTest.cpp

list_SOURCES = list.cpp

hashtable_SOURCES = hashtable.cpp

rand_SOURCES = rand.cpp

smartptr_SOURCES= smartptr.cpp

tree_SOURCES = tree.cpp

formatter_SOURCES = formatter.cpp

variant_SOURCES = variant.cpp

endif POSIX
