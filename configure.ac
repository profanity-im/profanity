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

AC_CHECK_PROG(RUBY_CMD_EXISTS, ruby, yes, no)
if test "$RUBY_CMD_EXISTS" == "yes"; then
    ac_mkmf_result=`ruby -rmkmf -e ";" 2>&1`
    if test -z "$ac_mkmf_result"; then
        AC_DEFUN([AX_WITH_RUBY],[
            AX_WITH_PROG(RUBY,ruby,$1,$2)
        ])
        AX_RUBY_DEVEL
        AM_CONDITIONAL([BUILD_RUBY_API], [true])
        AC_DEFINE([HAVE_RUBY], [1], [Ruby support])
    else
        AM_CONDITIONAL([BUILD_RUBY_API], [false])
        AC_MSG_NOTICE([Ruby development package not found, Ruby plugin support disabled.])
    fi
else
    AM_CONDITIONAL([BUILD_RUBY_API], [false])
    AC_MSG_NOTICE([Ruby not installed, Ruby plugin support disabled.])
fi

AC_CHECK_PROG(PYTHON_CONFIG_EXISTS, python-config, yes, no)
if test "$PYTHON_CONFIG_EXISTS" == "yes"; then
    AX_PYTHON_DEVEL
    AM_CONDITIONAL([BUILD_PYTHON_API], [true])
    AC_DEFINE([HAVE_PYTHON], [1], [Python support])
else
    AM_CONDITIONAL([BUILD_PYTHON_API], [false])
    AC_MSG_NOTICE([Python development package not found, Python plugin support disabled.])
fi

AX_PROG_LUA
AX_LUA_HEADERS
AX_LUA_LIBS
LUA_INCLUDE="-I/usr/include/lua5.1"
LUA_LIB="-llua5.1"
AM_CONDITIONAL([BUILD_LUA_API], [true])
AC_DEFINE([HAVE_LUA], [1], [Lua support])

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
AC_CHECK_LIB([dl], [main], [],
    [AC_MSG_ERROR([dl library needed to run C plugins])])

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
AM_CFLAGS="-Wall -export-dynamic"

if test "x$PACKAGE_STATUS" = xdevelopment; then
    AM_CFLAGS="$AM_CFLAGS -Wunused -Werror"
fi

LIBS="$LIBS $DEPS_LIBS $NOTIFY_LIBS"
AM_LDFLAGS="$AM_LDFLAGS $PYTHON_LDFLAGS $RUBY_LDFLAGS $LUA_LIB"
AM_CPPFLAGS="$DEPS_CFLAGS $NOTIFY_CFLAGS $PYTHON_CPPFLAGS $RUBY_CPPFLAGS $LUA_INCLUDE"

AC_SUBST(AM_LDFLAGS)
AC_SUBST(AM_CFLAGS)
AC_SUBST(AM_CPPFLAGS)

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([atexit memset strdup strstr])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
