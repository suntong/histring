
dnl Process this file with autoconf to produce a configure script.
AC_INIT
AC_CONFIG_SRCDIR([histring.c])
AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE(histring,1.1.0)


dnl Checks for programs.

AC_ARG_PROGRAM
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL

AC_CHECK_FUNCS( \
  getopt_long \
)

dnl Checks for header files.
AC_CHECK_HEADERS( \
  string.h \
  unistd.h \
  stdlib.h \
  errno.h \
  ctype.h \
  getopt.h \
)
AC_CHECK_HEADERS( 
  stdio.h \
  string.h \
  regex.h \
  ,,
  AC_MSG_WARN(could not locate neccessary system header files) \
)

dnl you need at least to have getopt, but getopt_long will be used if it
dnl is present
AC_CHECK_FUNCS(getopt)
if test "$ac_cv_func_getopt" != yes; then
  AC_MSG_ERROR(getopt is needed for this program to work)
fi

dnl include GNU getopt if you don't have a system getopt_long
AC_CHECK_FUNC(getopt_long, ,
    AC_DEFINE(HAVE_GETOPT_H)
    AC_DEFINE(HAVE_GETOPT_LONG)
    [EXTRASRC="$EXTRASRC \$(srcdir)/getopt.c \$(srcdir)/getopt1.c"]
    [EXTRAOBJ="$EXTRAOBJ \$(srcdir)/getopt.o \$(srcdir)/getopt1.o"] )

dnl check weather we want debugging support
dnl the default is to NOT use debugging support
AC_MSG_CHECKING(whether user wants debugging support)
AC_ARG_ENABLE(debug,
  [  --enable-debug          include support for debugging],
  [ AC_DEFINE(DEBUG)
    AC_MSG_RESULT(yes) ],
  [ AC_MSG_RESULT(no) ]   )

AC_SUBST(EXTRASRC)
AC_SUBST(EXTRAOBJ)

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
