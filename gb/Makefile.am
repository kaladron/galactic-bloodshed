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

noinst_LIBRARIES = libgblib.a

COMMON_FILES = files.cc files_rw.cc files_shl.cc sql/sql.cc utils/fileutils.cc

makeuniv_SOURCES = $(COMMON_FILES) creator/makeplanet.cc creator/makestar.cc creator/makeuniv.cc creator/namegen.cc perm.cc rand.cc sector.cc

enrol_SOURCES =  $(COMMON_FILES) enrol.cc max.cc perm.cc rand.cc sector.cc shlmisc.cc

GB_SOURCES = GB_server.cc
GB_LDADD = libgblib.a

libgblib_a_SOURCES = $(COMMON_FILES) VN.cc analysis.cc commands/autoreport.cc bombard.cc build.cc capital.cc \
		     capture.cc commands/colonies.cc commands/fix.cc commands/governors.cc commands/grant.cc \
		     commands/production.cc commands/zoom.cc cs.cc declare.cc dissolve.cc dock.cc dosector.cc \
		     doship.cc doturncmd.cc enslave.cc examine.cc explore.cc fire.cc fuel.cc getplace.cc \
		     land.cc launch.cc load.cc map.cc max.cc mobiliz.cc move.cc moveplanet.cc moveship.cc \
		     name.cc orbit.cc order.cc perm.cc planet.cc powercmd.cc prof.cc rand.cc relation.cc \
		     rst.cc scrap.cc sector.cc ships.cc shlmisc.cc shootblast.cc survey.cc \
		     tech.cc tele.cc toggle.cc toxicity.cc victory.cc

racegen_SOURCES = $(COMMON_FILES) GB_racegen.cc enroll.cc max.cc perm.cc racegen.cc rand.cc sector.cc shlmisc.cc

TESTS = shlmisc_test

check_PROGRAMS = shlmisc_test
shlmisc_test_SOURCES = shlmisc_test.cc
