#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])
AC_INIT([profanity], [0.1.9dev], [boothj5web@gmail.com])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([foreign subdir-objects])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB([ncurses], [main], [], 
    [AC_MSG_ERROR([ncurses is required for profanity])])
AC_CHECK_LIB([resolv], [main], [], 
    [AC_MSG_ERROR([libresolv is required for profanity])])
AC_CHECK_LIB([ssl], [main], [], 
    [AC_MSG_ERROR([openssl is required for profanity])])
AC_CHECK_LIB([expat], [main], [], 
    [AC_MSG_ERROR([expat is required for profanity])])
AC_CHECK_LIB([strophe], [main], [], 
    [AC_MSG_ERROR([libstrophe is required for profanity])])
AC_CHECK_LIB([glib-2.0], [main], [], 
    [AC_MSG_ERROR([glib-2.0 is required for profanity])])
AC_CHECK_LIB([curl], [main], [], 
    [AC_MSG_ERROR([libcurl is required for profanity])])
AC_CHECK_LIB([notify], [main], [], 
    [AC_MSG_NOTICE([libnotify not found, desktop notifications no supported])])
AC_CHECK_LIB([headunit], [main], [], 
    [AC_MSG_NOTICE([headunit not found, will not be able to run tests])])

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h])
AC_CHECK_HEADERS([ncurses.h], [], [])
AC_CHECK_HEADERS([ncurses/ncurses.h], [], [])

PKG_CHECK_MODULES([DEPS], [openssl glib-2.0 libcurl])
PKG_CHECK_MODULES([NOTIFY], [libnotify], [], 
    [AC_MSG_NOTICE([libnotify module not found])])

AM_CFLAGS="-Wall "
AM_CFLAGS="$AM_CFLAGS -lstrophe -lexpat -lncurses -lcurl -lresolv "
AM_CFLAGS="$AM_CFLAGS $DEPS_LIBS $NOTIFY_LIBS"

AM_CPPFLAGS="$DEPS_CFLAGS $NOTIFY_CFLAGS"

AC_SUBST(AM_CFLAGS)
AC_SUBST(AM_CPPFLAGS)

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([atexit memset strdup strndup strstr])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
