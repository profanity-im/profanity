#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([profanity], [0.10], [boothj5web@gmail.com])
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADERS([src/config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB([ncurses], [main], [], [AC_MSG_ERROR([ncurses is required for profanity])])
AC_CHECK_LIB([resolv], [main], [], [AC_MSG_ERROR([libresolv is required for profanity])])
AC_CHECK_LIB([ssl], [main], [], [AC_MSG_ERROR([openssl is required for profanity])])
AC_CHECK_LIB([strophe], [main], [], [AC_MSG_ERROR([libstrophe is required for profanity])])
AC_CHECK_LIB([expat], [main], [], [AC_MSG_ERROR([expat is required for profanity])])
AC_CHECK_LIB([xml2], [main], [], [AC_MSG_ERROR([xml2 is required for profanity])])
AC_CHECK_LIB([glib-2.0], [main], [], [AC_MSG_ERROR([glib-2.0 is required for profanity])])

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h])

PKG_CHECK_MODULES([OPENSSL], [openssl])
PKG_CHECK_MODULES([NCURSES], [ncurses])
PKG_CHECK_MODULES([GLIB], [glib-2.0])
PKG_CHECK_MODULES([NOTIFY], [libnotify], [], [AC_MSG_NOTICE([libnotify module not found])])

CFLAGS="$CFLAGS $GLIB_CFLAGS $GLIB_LIBS $NOTIFY_CFLAGS $NOTIFY_LIBS"
CPPFLAGS="$CPPFLAGS $GLIB_CFLAGS $GLIB_LIBS $NOTIFY_CFLAGS $NOTIFY_LIBS"

AC_CHECK_HEADERS([libnotify/notify.h], [], [AC_MSG_NOTICE([libnotify not found, desktop notification option not included])])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([atexit memset strdup strndup strstr])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
