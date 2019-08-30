
AM_CXXFLAGS = -std=c++2a -O0 -g3 \
	      $(SQLLITE3_CFLAGS) \
	      -DPKGSTATEDIR="\"$(PKGSTATEDIR)\"" \
	      -DPKGDATADIR="\"$(PKGDATADIR)\"" \
	      -DDOCDIR="\"$(DOCDIR)\""

if CLANG
AM_CXXFLAGS += $(CLANG_OPTS)
else
AM_CXXFLAGS += $(GCC_OPTS)
endif

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/external/GUnit/include

LIBS = $(SQLITE3_LIBS)

AUTOMAKE_OPTIONS = serial-tests

bin_PROGRAMS = storage_demos
noinst_LIBRARIES = libstorage.a

libstorage_a_SOURCES = memstore.cc \
					   schema.cc \
					   sqldb.cc	\
					   sqlstore.cc	\
					   store.cc \
					   types.cc \
					   utils.cc \
					   values.cc

storage_demos_SOURCES = demos/main.cc

storage_demos_LDADD = libstorage.a

TESTS = storage_test

check_PROGRAMS = storage_test

storage_test_SOURCES = tests/values.cc \
					   tests/types.cc \
					   tests/fieldpaths.cc \
					   tests/main.cc

storage_test_LDADD = $(top_srcdir)/external/googletest/lib/libgtest.a \
					 $(top_srcdir)/external/googletest/lib/libgtest_main.a \
					 $(top_srcdir)/external/googletest/lib/libgmock.a \
					 $(top_srcdir)/external/googletest/lib/libgmock_main.a \
					 libstorage.a


