AC_PREREQ([2.69])
AC_INIT([galactic bloodshed], [5.0], [jbailey@raspberryginger.com])
AM_INIT_AUTOMAKE([-Wall])
AC_CONFIG_SRCDIR([src/vars.h])
# AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CXX

AM_SILENT_RULES([yes])

# Checks for libraries.
# FIXME: Replace `main' with a function in `-lcurses':
AC_CHECK_LIB([curses], [main])
# FIXME: Replace `main' with a function in `-lm':
AC_CHECK_LIB([m], [main])
# FIXME: Replace `main' with a function in `-lsqlite3':
AC_CHECK_LIB([sqlite3], [main])
# FIXME: Replace `main' with a function in `-ltermcap':
AC_CHECK_LIB([termcap], [main])

AC_CONFIG_FILES([Makefile data/Makefile src/Makefile])
AC_OUTPUT
