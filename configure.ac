#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])
AC_INIT([profanity], [0.1.10], [boothj5web@gmail.com])
AC_DEFINE([PACKAGE_STATUS], ["development"], [Status of this build])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([foreign subdir-objects])

# Checks for programs.
AC_PROG_CC

# get canonical host
AC_CANONICAL_HOST
if test "$host_os" == "cygwin"; then
    AC_DEFINE([PLATFORM_CYGWIN], [1], [Cygwin])
fi

# Options
AC_ARG_WITH([libxml2],
    [AS_HELP_STRING([--with-libxml2], [link with libxml2 instead of expat])])

# Checks for libraries.
if test "x$with_libxml2" = xyes; then
    AC_CHECK_LIB([xml2], [main], [], 
        [AC_MSG_ERROR([libxml2 is required for profanity])])
else
    AC_CHECK_LIB([expat], [main], [], 
        [AC_MSG_ERROR([expat is required for profanity])])
fi

AC_CHECK_LIB([resolv], [main], [], 
    [AC_MSG_ERROR([libresolv is required for profanity])])
AC_CHECK_LIB([ssl], [main], [], 
    [AC_MSG_ERROR([openssl is required for profanity])])
AC_CHECK_LIB([strophe], [main], [], 
    [AC_MSG_ERROR([libstrophe is required for profanity])])
AC_CHECK_LIB([ncurses], [main], [], 
    [AC_MSG_ERROR([ncurses is required for profanity])])
AC_CHECK_LIB([glib-2.0], [main], [], 
    [AC_MSG_ERROR([glib-2.0 is required for profanity])])
AC_CHECK_LIB([curl], [main], [], 
    [AC_MSG_ERROR([libcurl is required for profanity])])
AC_CHECK_LIB([notify], [main], [], 
    [AC_MSG_NOTICE([libnotify not found, desktop notifications not supported])])
AC_CHECK_LIB([headunit], [main], [], 
    [AC_MSG_NOTICE([headunit not found, will not be able to run tests])])
AC_CHECK_LIB([Xss], [main], [], 
    [AC_MSG_NOTICE([libxss not found, falling back to profanity auto-away])])
AC_CHECK_LIB([X11], [main], [], 
    [AC_MSG_NOTICE([libX11 not found, falling back to profanity auto-away])])

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h])
AC_CHECK_HEADERS([ncurses.h], [], [])
AC_CHECK_HEADERS([ncurses/ncurses.h], [], [])

PKG_CHECK_MODULES([DEPS], [openssl glib-2.0 libcurl])
PKG_CHECK_MODULES([NOTIFY], [libnotify], [], 
    [AC_MSG_NOTICE([libnotify module not found])])

AM_CFLAGS="-Wall"
LIBS="$LIBS $DEPS_LIBS $NOTIFY_LIBS"

AM_CPPFLAGS="$DEPS_CFLAGS $NOTIFY_CFLAGS"

AC_SUBST(AM_CFLAGS)
AC_SUBST(AM_CPPFLAGS)

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([atexit memset strdup strndup strstr])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
