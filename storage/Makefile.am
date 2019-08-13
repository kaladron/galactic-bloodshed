
GCC_OPTS = -Wno-sign-compare -Wno-maybe-uninitialized -Wno-return-type -Wno-unused-variable
CLANG_OPTS = -Werror -Wno-c++98-compat -Wno-padded -Wno-c++98-compat-pedantic -Wno-float-equal \
    -Wno-old-style-cast -Wno-conversion -Wno-conditional-uninitialized -Wno-sign-compare \
    -Wno-switch-enum -Wno-exit-time-destructors -Wno-global-constructors -Wno-writable-strings
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

LIBS = $(SQLITE3_LIBS) $(PTHREAD_LIBS)

# bin_PROGRAMS = makeuniv enrol GB racegen
noinst_LIBRARIES = libstorage.a

libstorage_a_SOURCES = entities.cc \
					   memstore.cc \
					   schema.cc \
					   sqldb.cc	\
					   sqlstore.cc	\
					   store.cc \
					   types.cc \
					   values.cc

TESTS = storage_test

check_PROGRAMS = storage_test
storage_test_SOURCES = tests/values.cc \
					   tests/types.cc \
					   tests/main.cc

storage_test_LDADD = $(top_srcdir)/external/googletest/lib/libgtest.a \
					 $(top_srcdir)/external/googletest/lib/libgtest_main.a

