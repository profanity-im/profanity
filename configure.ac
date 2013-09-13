#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])
AC_INIT([profanity], [0.4.0], [boothj5web@gmail.com])
AC_CONFIG_AUX_DIR([build-aux])
LT_INIT
PACKAGE_STATUS="development"
AC_DEFINE_UNQUOTED([PACKAGE_STATUS], ["$PACKAGE_STATUS"], [Status of this build])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADERS([src/config.h])
AM_INIT_AUTOMAKE([foreign subdir-objects])
AX_PREFIX_CONFIG_H([src/prof_config.h], [PROF], [src/config.h])

# Checks for programs.
AC_PROG_CC

# get canonical host
AC_CANONICAL_HOST
if test "$host_os" == "cygwin"; then
    AC_DEFINE([PLATFORM_CYGWIN], [1], [Cygwin])
fi

# Options
AC_ARG_ENABLE([notifications],
    [AS_HELP_STRING([--enable-notifications], [enable desktop notifications])])
AC_ARG_WITH([libxml2],
    [AS_HELP_STRING([--with-libxml2], [link with libxml2 instead of expat])])
AC_ARG_WITH([xscreensaver],
    [AS_HELP_STRING([--with-xscreensaver], [use libXScrnSaver to determine indle time])])

# Checks for libraries.
if test "x$with_libxml2" = xyes; then
    AC_CHECK_LIB([xml2], [main], [],
        [AC_MSG_ERROR([libxml2 is required for profanity])])
else
    AC_CHECK_LIB([expat], [main], [],
        [AC_MSG_ERROR([expat is required for profanity])])
fi

if test "x$enable_notifications" = xyes; then
    AC_CHECK_LIB([notify], [main], [],
        [AC_MSG_ERROR([libnotify is required for desktop notifications])])
elif test "x$enable_notifications" = x; then
    AC_CHECK_LIB([notify], [main], [],
        [AC_MSG_NOTICE([libnotify not found, desktop notifications not supported])])
fi

if test "x$with_xscreensaver" = xyes; then
    AC_CHECK_LIB([Xss], [main], [],
        [AC_MSG_ERROR([libXss is required for x autoaway support])])
    AC_CHECK_LIB([X11], [main], [], 
        [AC_MSG_NOTICE([libX11 is required for x autoaway support])])
elif test "x$with_xscreensaver" = x; then
    AC_CHECK_LIB([Xss], [main], [],
        [AC_MSG_NOTICE([libXss not found, falling back to profanity auto-away])])
    AC_CHECK_LIB([X11], [main], [], 
        [AC_MSG_NOTICE([libX11 not found, falling back to profanity auto-away])])

fi

AC_CHECK_LIB([resolv], [main], [],
    [AC_MSG_ERROR([libresolv is required for profanity])])
AC_CHECK_LIB([ssl], [main], [],
    [AC_MSG_ERROR([openssl is required for profanity])])
AC_CHECK_LIB([strophe], [main], [],
    [AC_MSG_ERROR([libstrophe is required for profanity])])
AC_CHECK_LIB([ncursesw], [main], [],
    [AC_MSG_ERROR([ncursesw is required for profanity])])
AC_CHECK_LIB([glib-2.0], [main], [],
    [AC_MSG_ERROR([glib-2.0 is required for profanity])])
AC_CHECK_LIB([curl], [main], [],
    [AC_MSG_ERROR([libcurl is required for profanity])])
AC_CHECK_LIB([headunit], [main], [],
    [AC_MSG_NOTICE([headunit not found, will not be able to run tests])])

# check for python development, for python plugin support
AC_CHECK_PROG(PYTHON_CONFIG, python-config, yes, no)
if test $PYTHON_CONFIG = yes; then
    AC_MSG_NOTICE([Python supported.])
    AC_DEFINE([HAVE_PYTHON], [1], [Python])
    PYTHON_CFLAGS=`python-config --cflags`
    AC_MSG_NOTICE([Python CFLAGS=$PYTHON_CFLAGS])
    PYTHON_LIBS=`python-config --libs`
    AC_MSG_NOTICE([Python LIBS=$PYTHON_LIBS])
else
    AC_MSG_NOTICE([Python not supported])
fi

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h])

# Check for ncursesw/ncurses.h first, Arch linux uses ncurses.h for ncursesw
AC_CHECK_HEADERS([ncursesw/ncurses.h], [], [])
AC_CHECK_HEADERS([ncurses.h], [], [])

# Checks for pkgconfig modules
PKG_CHECK_MODULES([DEPS], [openssl glib-2.0 libcurl])

if test "x$enable_notifications" != xno; then
    PKG_CHECK_MODULES([NOTIFY], [libnotify], [],
        [AC_MSG_NOTICE([libnotify module not found])])
fi

# Default parameters
AM_CFLAGS="-Wall"

# TODO use another method to get flags for system
RUBY_CFLAGS="-I/usr/lib/ruby/1.8/x86_64-linux -L/usr/lib/ruby/1.8/x86_64-linux"
RUBY_LIBS="-lruby1.8"

# add python and ruby CFLAGS
AM_CFLAGS="$AM_CFLAGS $PYTHON_CFLAGS $RUBY_CFLAGS"

if test "x$PACKAGE_STATUS" = xdevelopment; then
#    AM_CFLAGS="$AM_CFLAGS -Wunused -Werror"
    AM_CFLAGS="$AM_CFLAGS -Wunused"
fi

LIBS="$LIBS $DEPS_LIBS $NOTIFY_LIBS $PYTHON_LIBS $RUBY_LIBS"

AM_CPPFLAGS="$DEPS_CFLAGS $NOTIFY_CFLAGS"

AC_SUBST(AM_CFLAGS)
AC_SUBST(AM_CPPFLAGS)

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([atexit memset strdup strstr])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
