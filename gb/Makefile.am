PKGSTATEDIR=$(libdir)/galactic-bloodshed/
PKGDATADIR=$(datarootdir)/galactic-bloodshed/
DOCDIR=$(datarootdir)/galactic-bloodshed/

install-data-local:
	echo "Creating folders..."
	mkdir -p $(PKGSTATEDIR)
	mkdir -p $(PKGSTATEDIR)/Tele
	mkdir -p $(PKGSTATEDIR)/News
	mkdir -p $(PKGDATADIR)
	mkdir -p $(DOCDIR)
	chmod -R a+rwx $(DOCDIR) $(PKGDATADIR) $(PKGSTATEDIR)
	cp $(top_srcdir)/data/* $(PKGDATADIR)/

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

AM_CPPFLAGS = -I$(top_srcdir)

LIBS = $(SQLITE3_LIBS)


bin_PROGRAMS = makeuniv enrol GB racegen

noinst_LIBRARIES = libgblib.a libcommands.a

COMMON_FILES = files.cc files_rw.cc files_shl.cc sql/sql.cc utils/fileutils.cc

makeuniv_SOURCES = $(COMMON_FILES) creator/makeplanet.cc creator/makestar.cc creator/makeuniv.cc creator/namegen.cc perm.cc rand.cc sector.cc

enrol_SOURCES =  $(COMMON_FILES) enrol.cc max.cc perm.cc rand.cc sector.cc shlmisc.cc

GB_SOURCES = GB_server.cc
GB_LDADD = libgblib.a libcommands.a

libcommands_a_SOURCES = \
		     commands/autoreport.cc \
		     commands/colonies.cc \
		     commands/enslave.cc \
		     commands/fix.cc \
		     commands/governors.cc \
		     commands/grant.cc \
		     commands/highlight.cc \
		     commands/production.cc \
		     commands/relation.cc \
		     commands/toggle.cc \
		     commands/toxicity.cc \
		     commands/zoom.cc

libgblib_a_SOURCES = $(COMMON_FILES) \
		     VN.cc \
		     analysis.cc \
		     bombard.cc \
		     build.cc \
		     capital.cc \
		     capture.cc \
		     cs.cc \
		     declare.cc \
		     dissolve.cc \
		     dock.cc \
		     dosector.cc \
		     doship.cc \
		     doturncmd.cc \
		     examine.cc \
		     explore.cc \
		     fire.cc \
		     fuel.cc \
		     getplace.cc \
		     land.cc \
		     launch.cc \
		     load.cc \
		     map.cc \
		     max.cc \
		     mobiliz.cc \
		     move.cc \
		     moveplanet.cc \
		     moveship.cc \
		     name.cc \
		     orbit.cc \
		     order.cc \
		     perm.cc \
		     doplanet.cc \
		     powercmd.cc \
		     prof.cc \
		     rand.cc \
		     rst.cc \
		     scrap.cc \
		     sector.cc \
		     ships.cc \
		     shlmisc.cc \
		     shootblast.cc \
		     survey.cc \
		     tech.cc \
		     tele.cc \
		     victory.cc

racegen_SOURCES = $(COMMON_FILES) GB_racegen.cc enroll.cc max.cc perm.cc racegen.cc rand.cc sector.cc shlmisc.cc

TESTS = shlmisc_test

check_PROGRAMS = shlmisc_test
shlmisc_test_SOURCES = shlmisc_test.cc
