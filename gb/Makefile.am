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

UTILS_FILES = utils/rand.cc utils/fileutils.cc
COMMON_FILES = files.cc files_rw.cc files_shl.cc sql/sql.cc $(UTILS_FILES)

makeuniv_SOURCES = $(COMMON_FILES)  \
					creator/makeplanet.cc \
					creator/makestar.cc \
					creator/makeuniv.cc \
					creator/namegen.cc \
					perm.cc \
					sector.cc

enrol_SOURCES =  $(COMMON_FILES) enrol.cc max.cc perm.cc sector.cc shlmisc.cc

GB_SOURCES = GB_server.cc
GB_LDADD = libgblib.a libcommands.a

libcommands_a_SOURCES = \
		     commands/analysis.cc \
		     commands/announce.cc \
		     commands/autoreport.cc \
		     commands/capital.cc \
		     commands/capture.cc \
		     commands/colonies.cc \
		     commands/dock.cc \
		     commands/enslave.cc \
		     commands/examine.cc \
		     commands/fix.cc \
		     commands/governors.cc \
		     commands/grant.cc \
		     commands/highlight.cc \
		     commands/mobilize.cc \
		     commands/orbit.cc \
		     commands/production.cc \
		     commands/relation.cc \
		     commands/tax.cc \
		     commands/tech_status.cc \
		     commands/technology.cc \
		     commands/rst.cc \
		     commands/scrap.cc \
		     commands/toggle.cc \
		     commands/toxicity.cc \
		     commands/zoom.cc

libgblib_a_SOURCES = $(COMMON_FILES) \
		     VN.cc \
		     bombard.cc \
		     build.cc \
		     cs.cc \
		     declare.cc \
		     dissolve.cc \
		     dosector.cc \
		     doship.cc \
		     doturncmd.cc \
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
		     order.cc \
		     perm.cc \
		     doplanet.cc \
		     place.cc \
		     powercmd.cc \
		     prof.cc \
		     sector.cc \
		     ships.cc \
		     shlmisc.cc \
		     shootblast.cc \
		     survey.cc \
		     tech.cc \
		     tele.cc \
		     victory.cc

racegen_SOURCES = $(COMMON_FILES) GB_racegen.cc enroll.cc max.cc perm.cc racegen.cc sector.cc shlmisc.cc

TESTS = shlmisc_test

check_PROGRAMS = shlmisc_test
shlmisc_test_SOURCES = shlmisc_test.cc
