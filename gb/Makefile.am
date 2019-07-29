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

makeuniv_SOURCES = files.cc files_rw.cc files_shl.cc makeplanet.cc makestar.cc makeuniv.cc perm.cc rand.cc sector.cc sql/sql.cc

enrol_SOURCES = enrol.cc files.cc files_rw.cc files_shl.cc max.cc perm.cc rand.cc sector.cc shlmisc.cc sql/sql.cc

GB_SOURCES = GB_server.cc
GB_LDADD = libgblib.a

libgblib_a_SOURCES = VN.cc analysis.cc autoreport.cc bombard.cc build.cc capital.cc capture.cc commands/colonies.cc commands/fix.cc \
		commands/governors.cc commands/grant.cc commands/production.cc commands/zoom.cc cs.cc declare.cc dissolve.cc \
		dock.cc dosector.cc doship.cc doturncmd.cc enslave.cc examine.cc explore.cc files.cc files_rw.cc files_shl.cc \
		fire.cc fuel.cc getplace.cc land.cc launch.cc load.cc map.cc max.cc mobiliz.cc move.cc moveplanet.cc moveship.cc \
		name.cc orbit.cc order.cc perm.cc planet.cc powercmd.cc prof.cc rand.cc relation.cc rst.cc scrap.cc sector.cc \
		ships.cc shlmisc.cc shootblast.cc sql/sql.cc survey.cc tech.cc tele.cc toggle.cc toxicity.cc victory.cc

racegen_SOURCES = GB_racegen.cc enroll.cc files.cc files_rw.cc files_shl.cc  \
		  max.cc perm.cc racegen.cc rand.cc sector.cc shlmisc.cc sql/sql.cc

TESTS = shlmisc_test

check_PROGRAMS = shlmisc_test
shlmisc_test_SOURCES = shlmisc_test.cc
