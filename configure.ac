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

# Add -Wextra to GCC_OPTS
AC_SUBST([GCC_OPTS], ["-Werror -Wall -Wno-sign-compare -Wno-maybe-uninitialized -Wno-return-type \
	   -Wno-format-overflow -Wno-nonnull -Wno-stringop-truncation -Wno-restrict \
	   -Wno-unused-but-set-variable -Wno-parentheses -Wno-write-strings \
	   -Wno-unused-variable -Wextra -Wno-unused-parameter -Wno-error=extra"])

AC_SUBST([CLANG_OPTS], ["\
  -Werror -Weverything -Wno-c++98-compat -Wno-padded \
  -Wno-c++98-compat-pedantic -Wno-float-equal -Wno-old-style-cast \
  -Wno-conversion -Wno-conditional-uninitialized -Wno-sign-compare \
  -Wno-switch-enum -Wno-exit-time-destructors -Wno-global-constructors \
  -Wno-signed-enum-bitfield -Wno-writable-strings -Wno-documentation-unknown-command \
  -Wno-weak-vtables"])

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
 storage/Makefile
])
AC_OUTPUT
