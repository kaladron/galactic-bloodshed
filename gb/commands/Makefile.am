AM_CXXFLAGS = -std=c++2a \
	      -fmodule-file=$(top_builddir)/gb/gblib.pcm \
	      -fsanitize=implicit-conversion 

# -fsanitize-undefined-trap-on-error

if CLANG
AM_CXXFLAGS += $(CLANG_OPTS)
else
AM_CXXFLAGS += $(GCC_OPTS)
endif

AM_CPPFLAGS = -I$(top_srcdir)

noinst_LIBRARIES = libcommands.a

EXTRA_libcommands_a_DEPENDENCIES = $(top_builddir)/gb/gblib.pcm commands.pcm
libcommands_a_CXXFLAGS = $(AM_CXXFLAGS) -fmodule-file=commands.pcm
libcommands_a_SOURCES = \
		     analysis.cc \
		     announce.cc \
		     autoreport.cc \
		     block.cc \
		     capital.cc \
		     capture.cc \
		     colonies.cc \
		     declare.cc \
		     dissolve.cc \
		     distance.cc \
		     dock.cc \
		     enslave.cc \
		     examine.cc \
		     explore.cc \
		     fix.cc \
		     governors.cc \
		     grant.cc \
		     highlight.cc \
		     invite.cc \
		     mobilize.cc \
		     orbit.cc \
		     pledge.cc \
		     power.cc \
		     production.cc \
		     relation.cc \
		     star_locations.cc \
		     tax.cc \
		     tech_status.cc \
		     technology.cc \
		     repair.cc \
		     rst.cc \
		     scrap.cc \
		     survey.cc \
		     toggle.cc \
		     toxicity.cc \
		     unpledge.cc \
		     victory.cc \
		     vote.cc \
		     zoom.cc

MOSTLYCLEANFILES = commands.pcm
BUILT_SOURCES = commands.pcm

commands.pcm: $(top_builddir)/gb/gblib.pcm

.cppm.pcm:
	$(CXX) $(AM_CPPFLAGS) $(AM_CXXFLAGS) --precompile $< -o $@

.pcm.o: 
	$(CXX) $(CPPFLAGS) -c $< -o $@
