
AM_CPPFLAGS = -DPKGDATADIR='"$(pkgdatadir)/"'

AM_CXXFLAGS = -g -O2 -W -Weverything -Werror \
	    -Wno-old-style-cast \
	    -Wno-logical-not-parentheses \
	    -Wno-conditional-uninitialized \
	    -Wno-float-equal \
	    -Wno-shadow \
	    -Wno-unreachable-code \
	    -Wno-conversion \
	    -Wno-unused-parameter \
	    -Wno-sign-compare \
	    -Wno-char-subscripts \
	    -Wno-tautological-constant-out-of-range-compare



# These warnings seem like ones we'll never enable
AM_CXXFLAGS += \
	    -Wno-disabled-macro-expansion \
	    -Wno-padded

bin_PROGRAMS = GB enrol makeuniv racegen

GB_SOURCES = \
	getplace.cc	\
	tele.cc		\
	autoreport.cc	\
	order.cc	\
	fuel.cc		\
	shootblast.cc	\
	explore.cc	\
	relation.cc	\
	mobiliz.cc	\
	shlmisc.cc	\
	enslave.cc	\
	capital.cc	\
	toggle.cc	\
	analysis.cc	\
	survey.cc	\
	files.cc	\
	zoom.cc		\
	perm.cc		\
	prof.cc		\
	GB_server.cc	\
	dissolve.cc	\
	name.cc		\
	fire.cc		\
	land.cc		\
	dock.cc		\
	tech.cc		\
	map.cc		\
	max.cc		\
	move.cc		\
	capture.cc	\
	cs.cc		\
	rst.cc		\
	misc.cc		\
	rand.cc		\
	load.cc		\
	scrap.cc	\
	build.cc	\
	orbit.cc	\
	victory.cc	\
	powercmd.cc	\
	launch.cc	\
	declare.cc	\
	examine.cc	\
	toxicity.cc	\
	get4args.cc	\
	files_rw.cc	\
	files_shl.cc 	\
	moveplanet.cc	\
	autoshoot.cc	\
	doplanet.cc	\
	doturncmd.cc	\
	doship.cc	\
	dosector.cc	\
	moveship.cc	\
	VN.cc

# the original enrollment program
enrol_SOURCES = \
	enrol.cc	\
	files_shl.cc	\
	files_rw.cc	\
	files.cc	\
	max.cc		\
	perm.cc		\
	rand.cc		\
	shlmisc.cc

# new-fangled enrollment program (more flexible than 'enrol')
racegen_SOURCES = \
	enroll.cc	\
	racegen.cc	\
	files_shl.cc	\
	files_rw.cc	\
	files.cc	\
	max.cc		\
	perm.cc		\
	rand.cc		\
	GB_racegen.cc	\
	shlmisc.cc

racegen_CXXFLAGS = -DPRIV $(AM_CXXFLAGS)
racegen_CXXFLAGS += \
	    -Wno-c++11-compat-deprecated-writable-strings

makeuniv_SOURCES = makeuniv.cc makestar.cc makeplanet.cc rand.cc perm.cc files.cc

