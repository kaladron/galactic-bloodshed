AM_CFLAGS = -g -O2 -W -Weverything -Werror \
	    -Wno-conditional-uninitialized \
	    -Wno-float-equal \
	    -Wno-bad-function-cast \
	    -Wno-missing-variable-declarations \
	    -Wno-shadow \
	    -Wno-unreachable-code \
	    -Wno-conversion \
	    -Wno-unused-parameter \
	    -Wno-sign-compare \
	    -Wno-array-bounds \
	    -Wno-char-subscripts \
	    -Wno-tautological-constant-out-of-range-compare

# These warnings seem like ones we'll never enable
AM_CFLAGS += \
	    -Wno-disabled-macro-expansion \
	    -Wno-padded

bin_PROGRAMS = GB enrol makeuniv racegen

GB_SOURCES = \
	getplace.c	\
	tele.c		\
	autoreport.c	\
	order.c		\
	fuel.c		\
	shootblast.c	\
	explore.c	\
	relation.c	\
	mobiliz.c	\
	shlmisc.c	\
	enslave.c	\
	capital.c	\
	toggle.c	\
	analysis.c	\
	survey.c	\
	files.c		\
	zoom.c		\
	perm.c		\
	prof.c		\
	GB_server.c	\
	dissolve.c	\
	name.c		\
	fire.c		\
	land.c		\
	dock.c		\
	tech.c		\
	map.c		\
	max.c		\
	move.c		\
	capture.c	\
	cs.c		\
	rst.c		\
	misc.c		\
	rand.c		\
	load.c		\
	scrap.c		\
	build.c		\
	orbit.c		\
	victory.c	\
	powercmd.c	\
	launch.c	\
	declare.c	\
	examine.c	\
	toxicity.c	\
	get4args.c	\
	files_rw.c	\
	files_shl.c 	\
	moveplanet.c	\
	autoshoot.c	\
	doplanet.c	\
	doturncmd.c	\
	doship.c	\
	dosector.c	\
	moveship.c	\
	VN.c

# the original enrollment program
enrol_SOURCES = \
	enrol.c		\
	files_shl.c	\
	files_rw.c	\
	files.c		\
	max.c		\
	perm.c		\
	rand.c		\
	shlmisc.c

# new-fangled enrollment program (more flexible than 'enrol')
racegen_SOURCES = \
	enroll.c	\
	racegen.c	\
	files_shl.c	\
	files_rw.c	\
	files.c		\
	max.c		\
	perm.c		\
	rand.c		\
	GB_racegen.c	\
	shlmisc.c

racegen_CFLAGS = -DPRIV

makeuniv_SOURCES = makeuniv.c makestar.c makeplanet.c rand.c perm.c files.c

