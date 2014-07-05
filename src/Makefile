# Galactic Bloodshed (Robert Chansky, smq@b)
# Makefile

# what version of UNIX we are running on.
#UV = VER_4_3
#UV = VER_4_2
# (the -pipe -g are for development.)
#CC = clang
CC = gcc

CFLAGS = -g -O2 -W -Wall -Werror \
	  -Wno-return-type \
          -Wno-unused-parameter \
	  -Wno-unused-variable \
	  -Wno-sign-compare \
	  -Wno-array-bounds \
	  -Wno-char-subscripts \
	  -Wno-type-limits

CFLAGS += \
	  -Wno-maybe-uninitialized \
	  -Wno-unused-but-set-variable 

#CFLAGS = -g -YBSD
#CFLAGS = -ansi -g3

#CFLAGS = -g3
#CFLAGS = -bsd -g
#CFLAGS = -bsd -g -W
#CFLAGS = -O -bsd -g
#CFLAGS = -D$(UV) -pipe -g 
#CFLAGS = -D$(UV) -O
#CFLAGS = -D$(UV) -O -g

# objects for shell
SHOBJS =  getplace.o	\
          tele.o	\
	autoreport.o	\
	order.o		\
	fuel.o		\
	shootblast.o	\
	explore.o	\
	relation.o	\
	mobiliz.o	\
	shlmisc.o	\
	enslave.o	\
	capital.o	\
	toggle.o	\
	analysis.o		\
	survey.o	\
	files.o		\
	zoom.o		\
	perm.o		\
	prof.o		\
	GB_server.o	\
	dissolve.o	\
	name.o		\
	fire.o		\
	land.o		\
	dock.o		\
	tech.o		\
	map.o		\
	max.o		\
	move.o		\
	capture.o	\
	cs.o		\
	rst.o		\
	misc.o		\
	rand.o		\
	load.o		\
	scrap.o		\
	build.o		\
	orbit.o		\
	victory.o	\
	power.o		\
	launch.o	\
	declare.o	\
	examine.o	\
	toxicity.o	\
	get4args.o	\
	files_rw.o	\
	files_shl.o 	\
	moveplanet.o	\
	autoshoot.o	\
	doplanet.o	\
	doturn.o	\
	doship.o	\
	dosector.o	\
	moveship.o	\
	VN.o


# depend on vars.h and races.h
VROBJS = getplace.o	\
	dissolve.o	\
	autoreport.o	\
	order.o	\
	mobiliz.o	\
	shlmisc.o	\
	capital.o	\
	toggle.o	\
	enrol.o	\
	max.o		\
	move.o		\
	capture.o	\
	zoom.o		\
	toxicity.o	\
	doplanet.o	\
	shootblast.o	\
	autoshoot.o	\
	moveship.o	\
	explore.o	\
	relation.o	\
	dosector.o	\
	declare.o	\
	analysis.o		\
	survey.o	\
	prof.o		\
	perm.o		\
	rst.o		\
	GB_server.o	\
	VN.o		\
	cs.o		\
	max.o		\
	map.o		\
	scrap.o		\
	name.o		\
	fire.o		\
	victory.o	\
	power.o		\
	doturn.o	\
	doship.o	\
	makeuniv.o	\
	makeplanet.o


# depend on ships.h and vars.h
SHIPOBJS = autoreport.o	\
	dissolve.o	\
	order.o	\
	shootblast.o	\
	makeplanet.o	\
	enrol.o	\
	examine.o	\
	capital.o	\
	enslave.o	\
	relation.o	\
	capital.o	\
	analysis.o		\
	survey.o	\
	launch.o	\
	build.o		\
	enrol.o		\
	fire.o		\
	land.o		\
	dock.o		\
	map.o		\
	max.o		\
	move.o		\
	capture.o	\
	cs.o		\
	VN.o		\
	rst.o		\
	load.o		\
	name.o		\
	prof.o		\
	scrap.o		\
	orbit.o		\
	doturn.o	\
	doship.o	\
	doplanet.o	\
	moveship.o	\
	autoshoot.o	\
	moveplanet.o	\
	files_shl.o	\
	victory.o

# depend on files.h
FOBJS =	files_shl.o 	\
	files_rw.o	\
	makeuniv.o	\
	shlmisc.o 	\
	doturn.o 	\
	enrol.o 	\
	files.o		\
	GB_server.o	\
	build.o		\
	power.o 	\
	victory.o	\
	daemon.o	\
	examine.o

# dont depend on anything
NOOBJS = get4args.o	\
	misc.o		\
	rand.o		\
	daemon.o	\
	files_rw.o

# objects for enrol
EOBJS = enrol.o	\
	files_shl.o	\
	files_rw.o		\
	files.o	\
	max.o	\
	perm.o	\
	rand.o	\
	shlmisc.o

#objects for racegen (more flexible than 'enrol')
RACEGENOBJS = enroll.o \
	racegen.o	\
	files_shl.o	\
	files_rw.o	\
	files.o	\
	max.o	\
	perm.o	\
	rand.o	\
	GB_racegen.o	\
	shlmisc.o

# objects for guardian
GUARDOBJS = guardian.o

all: GB enrol racegen makeuniv guardian

clean:
	-rm -f *.o enrol racegen makeuniv guardian

# the shell 
GB : ${SHOBJS}
	${CC} -o GB ${SHOBJS}  -lcurses -ltermcap -lm;

${VROBJS}: races.h vars.h
${SHIPOBJS}: vars.h ships.h
${FOBJS}: files.h
build.o : vars.h ships.h races.h shipdata.h
victory power.o dissolve.o files_shl.o : power.h
dissolve.o doturn.o doship.o doplanet.o moveship.o moveplanet.o : doturn.h
moveship.o : vars.h ships.h shipdata.h
doturn.o doplanet.o doship.o : power.h


#guardian program
guardian : ${GUARDOBJS}
	${CC} -o guardian ${GUARDOBJS} -lcurses -ltermcap -lm

#sizes : vars.h ships.h races.h
#	 sizes.c

# make data files program
makeuniv : makeuniv.o makestar.o makeplanet.o rand.o perm.o files.o
	${CC} -o makeuniv makeuniv.o makestar.o makeplanet.o rand.o perm.o files.o -lcurses -ltermcap -lm
makeuniv.o : power.h

# the origianl enrollment program
enrol : ${EOBJS}
	${CC} -o enrol ${EOBJS} -lcurses -ltermcap -lm
enrol.o : vars.h ships.h shipdata.h races.h

# new-fangled enrollment program
racegen.o : racegen.c
	${CC} $(CFLAGS) -DPRIV racegen.c -c
racegen : ${RACEGENOBJS}
	${CC} -o racegen ${RACEGENOBJS} -lm


# shar it up for transport
shar:
	shar FAQ* README Makefile GBMakingInstructions *.h *.c *.list *.dat *.txt *.p > GB.shar





