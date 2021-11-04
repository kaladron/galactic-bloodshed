AC_INIT([gb], [1.0], [gb@testmail.org])
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])
AM_PROG_AR
AC_PROG_CXX
have_clang=no
AC_MSG_CHECKING([if clang is the compiler])
AC_RUN_IFELSE([AC_LANG_PROGRAM([],[
  #if __clang__ != 1
  exit(1);
  #endif
])], [
have_clang=yes
])

AC_ARG_ENABLE(debug,
AS_HELP_STRING([--enable-debug],
               [turn on debug mode]),
[case "${enableval}" in
             yes) debug=true ;;
             no)  debug=false ;;
             *)   AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
esac],
[debug=false])

AM_CONDITIONAL(DEBUG, test x"$debug" = x"true")

if test x"$debug" = x"true"; then
  AC_SUBST([CXXFLAGS], ["-g3 -O0 -D_GLIBCXX_DEBUG -fsanitize=undefined -fdebug-macro -fsanitize=address -fno-omit-frame-pointer -fno-optimize-sibling-calls"])
fi

# Add -Wextra to GCC_OPTS
AC_SUBST([GCC_OPTS], ["-Werror -Wall -Wno-sign-compare -Wno-maybe-uninitialized -Wno-return-type \
	   -Wno-format-overflow -Wno-nonnull -Wno-stringop-truncation -Wno-restrict \
	   -Wno-unused-but-set-variable -Wno-parentheses -Wno-write-strings \
	   -Wno-unused-variable -Wextra -Wno-unused-parameter -Wno-error=extra"])

clang_opts="-fmodules"
clang_opts="${clang_opts} -stdlib=libc++"

clang_opts="${clang_opts} -Werror" # No stinkin' warnings allowed here!
clang_opts="${clang_opts} -Weverything" # Turn on all warnings

# Warnings we will probably never turn off
clang_opts="${clang_opts} -Wno-c++98-compat"
clang_opts="${clang_opts} -Wno-c++98-compat-pedantic"
clang_opts="${clang_opts} -Wno-padded"
clang_opts="${clang_opts} -Wno-switch-enum"

# Things we should get rid of
clang_opts="${clang_opts} -Wno-float-equal"
clang_opts="${clang_opts} -Wno-old-style-cast"
clang_opts="${clang_opts} -Wno-conversion"
clang_opts="${clang_opts} -Wno-conditional-uninitialized"
clang_opts="${clang_opts} -Wno-sign-compare"
clang_opts="${clang_opts} -Wno-exit-time-destructors"
clang_opts="${clang_opts} -Wno-global-constructors"
clang_opts="${clang_opts} -Wno-weak-vtables"
clang_opts="${clang_opts} -Wno-reserved-identifier"

AC_SUBST([CLANG_OPTS], ["${clang_opts}"])

AC_CHECK_LIB(pthread, pthread_create, [PTHREAD_LIBS+=-lpthread])
AM_CONDITIONAL([CLANG], [test x$have_clang = xyes])
AC_MSG_RESULT($have_clang)
AC_PROG_RANLIB
PKG_CHECK_MODULES(SQLITE3, sqlite3, ,
		  AC_ERROR([Sqlite3 is required to build GB]))
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
 Makefile
 gb/Makefile
 gb/commands/Makefile
 storage/Makefile
])
AC_OUTPUT
