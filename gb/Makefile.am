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

AM_CXXFLAGS = -std=c++2a \
	      $(SQLLITE3_CFLAGS) \
	      -DPKGSTATEDIR="\"$(PKGSTATEDIR)\"" \
	      -DPKGDATADIR="\"$(PKGDATADIR)\"" \
	      -DDOCDIR="\"$(DOCDIR)\"" \
	      -fsanitize=implicit-conversion 

# -fsanitize-undefined-trap-on-error

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

makeuniv_CXXFLAGS = $(AM_CXXFLAGS) -fmodule-file=gblib.pcm
makeuniv_LDADD = libgblib.a
makeuniv_SOURCES = $(COMMON_FILES)  \
					creator/makeplanet.cc \
					creator/makestar.cc \
					creator/makeuniv.cc \
					creator/namegen.cc \
					perm.cc \
					sectormap.cc

enrol_CXXFLAGS = $(AM_CXXFLAGS) -fmodule-file=gblib.pcm
enrol_SOURCES =  $(COMMON_FILES) enrol.cc max.cc perm.cc sectormap.cc shlmisc.cc
enrol_LDADD = libgblib.a

GB_SOURCES = GB_server.cc $(COMMON_FILES) $(OLD_LIBGBLIB_FILES)
GB_CXXFLAGS = $(AM_CXXFLAGS) -fmodule-file=gblib.pcm
GB_LDADD = libgblib.a libcommands.a

libcommands_a_CXXFLAGS = $(AM_CXXFLAGS) -fmodule-file=gblib.pcm
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
		     commands/victory.cc \
		     commands/zoom.cc

libgblib_a_CXXFLAGS = $(AM_CXXFLAGS) -fmodule-file=gblib.pcm
libgblib_a_SOURCES = sector.cc


OLD_LIBGBLIB_FILES = \
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
		     move.cc \
		     moveplanet.cc \
		     moveship.cc \
		     name.cc \
		     order.cc \
		     perm.cc \
		     doplanet.cc \
		     planet.cc \
		     place.cc \
		     powercmd.cc \
		     prof.cc \
		     sectormap.cc \
		     ships.cc \
		     shlmisc.cc \
		     shootblast.cc \
		     star.cc \
		     survey.cc \
		     tech.cc \
		     tele.cc \
		     victory.cc

racegen_CXXFLAGS = $(AM_CXXFLAGS) -fmodule-file=gblib.pcm -DPRIV
racegen_SOURCES = $(COMMON_FILES) GB_racegen.cc enroll.cc max.cc perm.cc racegen.cc sectormap.cc shlmisc.cc
racegen_LDADD = libgblib.a

TESTS = shlmisc_test

check_PROGRAMS = shlmisc_test
shlmisc_test_CXXFLAGS = $(AM_CXXFLAGS) -fmodule-file=gblib.pcm
shlmisc_test_SOURCES = shlmisc_test.cc
shlmisc_test_LDADD = libgblib.a

MOSTLYCLEANFILES = gblib.pcm
BUILT_SOURCES = gblib.pcm

.cppm.pcm:
	$(CXX) $(AM_CPPFLAGS) $(AM_CXXFLAGS) --precompile $< -o $@

.pcm.o: 
	$(CXX) $(CPPFLAGS) -c $< -o $@
