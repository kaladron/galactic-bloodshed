
AM_CPPFLAGS = -DPKGSTATEDIR='"$(pkgdatadir)/"'

AM_CXXFLAGS = -std=c++11 -W -Weverything -Werror \
	    -Wno-old-style-cast \
	    -Wno-logical-not-parentheses \
	    -Wno-conditional-uninitialized \
	    -Wno-float-equal \
	    -Wno-shadow \
	    -Wno-unreachable-code \
	    -Wno-unused-parameter \
	    -Wno-sign-compare \
	    -Wno-tautological-constant-out-of-range-compare

AM_CXXFLAGS += \
	    -Wno-conversion

# These warnings seem like ones we'll never enable
AM_CXXFLAGS += \
	    -Wno-switch-enum \
	    -Wno-disabled-macro-expansion \
	    -Wno-c++98-compat \
	    -Wno-padded

AM_CXXFLAGS += -fsanitize=address
CXXFLAGS = -g -O0 -fPIE
#AM_CXXFLAGS += -fsanitize=memory -fno-omit-frame-pointer -fno-optimize-sibling-calls

bin_PROGRAMS = GB enrol makeuniv racegen

GB_SOURCES = \
	GB_server.cc	\
	VN.cc		\
	analysis.cc	\
	autoreport.cc	\
	autoshoot.cc	\
	build.cc	\
	capital.cc	\
	capture.cc	\
	cs.cc		\
	declare.cc	\
	dissolve.cc	\
	dock.cc		\
	doplanet.cc	\
	dosector.cc	\
	doship.cc	\
	doturncmd.cc	\
	enslave.cc	\
	examine.cc	\
	explore.cc	\
	files.cc	\
	files_rw.cc	\
	files_shl.cc 	\
	fire.cc		\
	fuel.cc		\
	get4args.cc	\
	getplace.cc	\
	land.cc		\
	launch.cc	\
	load.cc		\
	map.cc		\
	max.cc		\
	misc.cc		\
	mobiliz.cc	\
	move.cc		\
	moveplanet.cc	\
	moveship.cc	\
	name.cc		\
	orbit.cc	\
	order.cc	\
	perm.cc		\
	powercmd.cc	\
	prof.cc		\
	rand.cc		\
	relation.cc	\
	rst.cc		\
	scrap.cc	\
	shlmisc.cc	\
	shootblast.cc	\
	survey.cc	\
	tech.cc		\
	tele.cc		\
	toggle.cc	\
	toxicity.cc	\
	victory.cc	\
	zoom.cc	

# the original enrollment program
enrol_SOURCES = \
	enrol.cc	\
	files.cc	\
	files_rw.cc	\
	files_shl.cc	\
	max.cc		\
	perm.cc		\
	rand.cc		\
	shlmisc.cc

# new-fangled enrollment program (more flexible than 'enrol')
racegen_SOURCES = \
	GB_racegen.cc	\
	enroll.cc	\
	files.cc	\
	files_rw.cc	\
	files_shl.cc	\
	max.cc		\
	perm.cc		\
	racegen.cc	\
	rand.cc		\
	shlmisc.cc

racegen_CXXFLAGS = -DPRIV $(AM_CXXFLAGS)
racegen_CXXFLAGS += \
	    -Wno-c++11-compat-deprecated-writable-strings

makeuniv_SOURCES = \
	 files.cc \
	 files_rw.cc \
	 files_shl.cc \
	 makeuniv.cc \
	 makestar.cc \
	 makeplanet.cc \
	 perm.cc \
	 rand.cc

run-%: %
	ASAN_SYMBOLIZER_PATH=/usr/bin/llvm-symbolizer-3.5 MSAN_SYMBOLIZER_PATH=/usr/bin/llvm-symbolizer-3.5 ASAN_OPTIONS="detect_leaks=1" ./$<
