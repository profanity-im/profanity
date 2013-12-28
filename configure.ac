#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])
AC_INIT([profanity], [0.4.0], [boothj5web@gmail.com])
PACKAGE_STATUS="development"

## get git branch and revision if in development
if test "x$PACKAGE_STATUS" = xdevelopment; then
    AM_CONDITIONAL([INCLUDE_GIT_VERSION], [true])
    AC_DEFINE([HAVE_GIT_VERSION], [1], [Include git info])
else
    AM_CONDITIONAL([INCLUDE_GIT_VERSION], [false])
fi

AC_DEFINE_UNQUOTED([PACKAGE_STATUS], ["$PACKAGE_STATUS"], [Status of this build])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([foreign subdir-objects])

# Checks for programs.
AC_PROG_CC

# get canonical host
AC_CANONICAL_HOST
AS_IF([test "x$host_os" = xcygwin],
    [AC_DEFINE([PLATFORM_CYGWIN], [1], [Cygwin])])

# Options
AC_ARG_ENABLE([notifications],
    [AS_HELP_STRING([--enable-notifications], [enable desktop notifications])])
AC_ARG_WITH([libxml2],
    [AS_HELP_STRING([--with-libxml2], [link with libxml2 instead of expat])])
AC_ARG_WITH([xscreensaver],
    [AS_HELP_STRING([--with-xscreensaver], [use libXScrnSaver to determine idle time])])

PARSER=""
AS_IF([test "x$with_libxml2" != xyes],
    [PKG_CHECK_MODULES([expat], [expat],
        [PARSER_CFLAGS="$expat_CFLAGS"; PARSER_LIBS="$expat_LIBS"; PARSER="expat"],
        AS_IF([test "x$with_libxml2" = xno],
            [AC_MSG_ERROR([expat is required but does not exist])]))
    ])

AS_IF([test "x$PARSER" = x -a "x$with_libxml2" != xno],
    [PKG_CHECK_MODULES([libxml2], [libxml-2.0],
        [PARSER_CFLAGS="$libxml2_CFLAGS"; PARSER_LIBS="$libxml2_LIBS"; PARSER="libxml2"],
        AS_IF([test "x$with_libxml2" = xyes],
            [AC_MSG_ERROR([libxml2 is required but does not exist])]))
    ])

AS_IF([test "x$PARSER" = x],
    [AC_MSG_ERROR([either expat or libxml2 is required for profanity])])

AC_CHECK_LIB([resolv], [__res_query], [],
    [AC_MSG_ERROR([libresolv is required for profanity])])
AC_CHECK_LIB([strophe], [main], [],
    [AC_MSG_ERROR([libstrophe is required for profanity])])
PKG_CHECK_MODULES([openssl], [openssl], [],
    [AC_MSG_ERROR([openssl is required for profanity])])
PKG_CHECK_MODULES([glib], [glib-2.0], [],
    [AC_MSG_ERROR([glib is required for profanity])])
PKG_CHECK_MODULES([curl], [libcurl], [],
    [AC_MSG_ERROR([libcurl is required for profanity])])
PKG_CHECK_MODULES([ncursesw], [ncursesw],
    [NCURSES_CFLAGS="$ncursesw_CFLAGS"; NCURSES_LIBS="$ncursesw_LIBS"],
    [PKG_CHECK_MODULES([ncurses], [ncurses],
        [NCURSES_CFLAGS="$ncurses_CFLAGS"; NCURSES_LIBS="$ncurses_LIBS"],
        [AC_MSG_ERROR([ncurses is required for profanity])])])

AS_IF([test "x$enable_notifications" != xno],
    [PKG_CHECK_MODULES([libnotify], [libnotify],
        [AC_DEFINE([HAVE_LIBNOTIFY], [1], [libnotify module])],
        [AS_IF([test "x$enable_notifications" = xyes],
            [AC_MSG_ERROR([libnotify is required but does not exist])],
            [AC_MSG_NOTICE([libnotify support will be disabled])])])])

# TODO: rewrite this
if test "x$with_xscreensaver" = xyes; then
    AC_CHECK_LIB([Xss], [main], [],
        [AC_MSG_ERROR([libXss is required for x autoaway support])])
    AC_CHECK_LIB([X11], [main], [],
        [AC_MSG_ERROR([libX11 is required for x autoaway support])])
elif test "x$with_xscreensaver" = x; then
    AC_CHECK_LIB([Xss], [main], [],
        [AC_MSG_NOTICE([libXss not found, falling back to profanity auto-away])])
    AC_CHECK_LIB([X11], [main], [],
        [AC_MSG_NOTICE([libX11 not found, falling back to profanity auto-away])])
fi

PKG_CHECK_MODULES([cmocka], [cmocka], [],
    [AC_MSG_NOTICE([cmocka is not found, will not be able to run tests])])

# Check for ncursesw/ncurses.h first, Arch linux uses ncurses.h for ncursesw
AC_CHECK_HEADERS([ncursesw/ncurses.h], [], [])
AC_CHECK_HEADERS([ncurses.h], [], [])

# Default parameters
AM_CFLAGS="-Wall"
AS_IF([test "x$PACKAGE_STATUS" = xdevelopment],
    [AM_CFLAGS="$AM_CFLAGS -Wunused -Werror"])
AM_CPPFLAGS="$openssl_CFLAGS $glib_CFLAGS $curl_CFLAGS $libnotify_CFLAGS"
AM_CPPFLAGS="$AM_CPPFLAGS $PARSER_CFLAGS $NCURSES_CFLAGS"
LIBS="$LIBS $openssl_LIBS $glib_LIBS $curl_LIBS $libnotify_LIBS"
LIBS="$LIBS $PARSER_LIBS $NCURSES_LIBS"

AC_SUBST(AM_CFLAGS)
AC_SUBST(AM_CPPFLAGS)

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([atexit memset strdup strstr])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

echo ""
echo "PACKAGE_STATUS : $PACKAGE_STATUS"
echo "AM_CFLAGS      : $AM_CFLAGS"
echo "AM_CPPFLAGS    : $AM_CPPFLAGS"
echo "LIBS           : $LIBS"
echo "XML Parser     : $PARSER"
echo ""
echo "Now you can run \`make' to build profanity"
