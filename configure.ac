#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_INIT([profanity], [0.6.0], [boothj5web@gmail.com])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADERS([src/config.h])
AM_INIT_AUTOMAKE([foreign subdir-objects])
# TODO Introduce with Ruby plugins, see https://github.com/boothj5/profanity/issues/779
#AX_PREFIX_CONFIG_H([src/prof_config.h], [PROF], [src/config.h])

### Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL

## Check for LFS
AC_SYS_LARGEFILE

### Get canonical host
AC_CANONICAL_HOST
PLATFORM="unknown"
AS_CASE([$host_os],
    [freebsd*], [PLATFORM="freebsd"],
    [darwin*], [PLATFORM="osx"],
    [cygwin], [PLATFORM="cygwin"],
    [PLATFORM="nix"])

PACKAGE_STATUS="development"

### Get git branch and revision if in development
if test "x$PACKAGE_STATUS" = xdevelopment; then
    AM_CONDITIONAL([INCLUDE_GIT_VERSION], [true])
    AC_DEFINE([HAVE_GIT_VERSION], [1], [Include git info])
else
    AM_CONDITIONAL([INCLUDE_GIT_VERSION], [false])
fi

AC_DEFINE_UNQUOTED([PACKAGE_STATUS], ["$PACKAGE_STATUS"], [Status of this build])

AS_IF([test "x$PLATFORM" = xcygwin],
    [AC_DEFINE([PLATFORM_CYGWIN], [1], [Cygwin])])
AS_IF([test "x$PLATFORM" = xosx],
    [AC_DEFINE([PLATFORM_OSX], [1], [OSx])])

### Environment variables
AC_ARG_VAR([PYTHON_FRAMEWORK], [Set base directory for Python Framework])

### Options
AC_ARG_ENABLE([notifications],
    [AS_HELP_STRING([--enable-notifications], [enable desktop notifications])])
AC_ARG_ENABLE([python-plugins],
    [AS_HELP_STRING([--enable-python-plugins], [enable Python plugins])])
AC_ARG_ENABLE([c-plugins],
    [AS_HELP_STRING([--enable-c-plugins], [enable C plugins])])
AC_ARG_ENABLE([plugins],
    [AS_HELP_STRING([--enable-plugins], [enable plugins])])
AC_ARG_ENABLE([otr],
    [AS_HELP_STRING([--enable-otr], [enable otr encryption])])
AC_ARG_ENABLE([pgp],
    [AS_HELP_STRING([--enable-pgp], [enable pgp])])
AC_ARG_WITH([xscreensaver],
    [AS_HELP_STRING([--with-xscreensaver], [use libXScrnSaver to determine idle time])])
AC_ARG_WITH([themes],
    [AS_HELP_STRING([--with-themes[[=PATH]]], [install themes (default yes)])])
AC_ARG_ENABLE([icons],
    [AS_HELP_STRING([--enable-icons], [enable GTK tray icons])])

### plugins

# python
if test "x$enable_plugins" = xno; then
    AM_CONDITIONAL([BUILD_PYTHON_API], [false])
elif test "x$enable_python_plugins" != xno; then
    AS_IF([test "x$PLATFORM" = xosx], [
        AS_IF([test "x$PYTHON_FRAMEWORK" = x], [ PYTHON_FRAMEWORK="/Library/Frameworks/Python.framework" ])
        AC_MSG_NOTICE([Symlinking Python.framework to $PYTHON_FRAMEWORK])
        rm -f Python.framework
        ln -s $PYTHON_FRAMEWORK Python.framework ])
    AC_CHECK_PROG(PYTHON_CONFIG_EXISTS, python-config, yes, no)
    if test "$PYTHON_CONFIG_EXISTS" == "yes"; then
        AX_PYTHON_DEVEL
        AM_CONDITIONAL([BUILD_PYTHON_API], [true])
        AC_DEFINE([HAVE_PYTHON], [1], [Python support])
    else
        if test "x$enable_python_plugins" = xyes; then
            AC_MSG_ERROR([Python not found, cannot enable Python plugins.])
        else
            AM_CONDITIONAL([BUILD_PYTHON_API], [false])
            AC_MSG_NOTICE([Python development package not found, Python plugin support disabled.])
        fi
    fi
    AS_IF([test "x$PLATFORM" = xosx], [rm -f Python.framework])
else
    AM_CONDITIONAL([BUILD_PYTHON_API], [false])
fi

# c
if test "x$PLATFORM" = xcygwin; then
    AM_CONDITIONAL([BUILD_C_API], [false])
else
    LT_INIT
    if test "x$enable_plugins" = xno; then
        AM_CONDITIONAL([BUILD_C_API], [false])
    elif test "x$enable_c_plugins" != xno; then
        AC_CHECK_LIB([dl], [main],
            [AM_CONDITIONAL([BUILD_C_API], [true]) LIBS="$LIBS -ldl" AC_DEFINE([HAVE_C], [1], [C support])],
            [AS_IF(
                [test "x$enable_c_plugins" = xyes],
                    [AC_MSG_ERROR([dl library needed to run C plugins])],
                [AM_CONDITIONAL([BUILD_C_API], [false])])
            ])
    else
        AM_CONDITIONAL([BUILD_C_API], [false])
    fi
fi

# threading
ACX_PTHREAD([], [AC_MSG_ERROR([pthread is required])])
LIBS="$PTHREAD_LIBS $LIBS"
CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
AS_IF([test "x$PTHREAD_CC" != x], [ CC="$PTHREAD_CC" ])

### Check for libmesode, fall back to libstrophe
PKG_CHECK_MODULES([libmesode], [libmesode],
    [LIBS="$libmesode_LIBS $LIBS" CFLAGS="$CFLAGS $libmesode_CFLAGS" AC_DEFINE([HAVE_LIBMESODE], [1], [libmesode])],
    [PKG_CHECK_MODULES([libstrophe], [libstrophe],
        [LIBS="$libstrophe_LIBS $LIBS" CFLAGS="$CFLAGS $libstrophe_CFLAGS" AC_DEFINE([HAVE_LIBSTROPHE], [1], [libstrophe])],
        [AC_MSG_ERROR([Neither libmesode or libstrophe found, either is required for profanity])])])

### Check for ncurses library
PKG_CHECK_MODULES([ncursesw], [ncursesw],
    [NCURSES_CFLAGS="$ncursesw_CFLAGS"; NCURSES_LIBS="$ncursesw_LIBS"; NCURSES="ncursesw"],
    [PKG_CHECK_MODULES([ncurses], [ncurses],
        [NCURSES_CFLAGS="$ncurses_CFLAGS"; NCURSES_LIBS="$ncurses_LIBS"; NCURSES="ncurses"],
        [AC_CHECK_LIB([ncursesw], [main], [],
            [AC_CHECK_LIB([ncurses], [main], [],
            	[AC_MSG_ERROR([ncurses is required for profanity])])])])])
AM_CPPFLAGS="$AM_CPPFLAGS $NCURSES_CFLAGS"
LIBS="$NCURSES_LIBS $LIBS"

### Check wide characters support in ncurses library
CFLAGS_RESTORE="$CFLAGS"
CFLAGS="$CFLAGS $NCURSES_CFLAGS"
AC_CACHE_CHECK([for wget_wch support in $NCURSES], ncurses_cv_wget_wch,
   [AC_LINK_IFELSE([AC_LANG_SOURCE([
       void wget_wch(void);
       int main() {
           wget_wch();
           return 0;
       }
       ])],
       [ncurses_cv_wget_wch=yes],
       [ncurses_cv_wget_wch=no])
   ])
CFLAGS="$CFLAGS_RESTORE"

AS_IF([test "x$ncurses_cv_wget_wch" != xyes],
    [AC_MSG_ERROR([ncurses does not support wide characters])])

### Check for other profanity dependencies
PKG_CHECK_MODULES([glib], [glib-2.0 >= 2.40], [],
    [AC_MSG_ERROR([glib 2.40 or higher is required for profanity])])
PKG_CHECK_MODULES([curl], [libcurl], [],
    [AC_CHECK_LIB([curl], [main], [],
        [AC_MSG_ERROR([libcurl is required for profanity])])])

AS_IF([test "x$enable_icons" != xno],
    [PKG_CHECK_MODULES([GTK], [gtk+-2.0 >= 2.24.10],
        [AC_DEFINE([HAVE_GTK], [1], [libgtk module])],
        [AS_IF([test "x$enable_icons" = xyes],
            [AC_MSG_ERROR([gtk+-2.0 or higher is required for icons])],
            [AC_MSG_NOTICE([gtk+-2.0 not found, icons not enabled])])])])

AS_IF([test "x$PLATFORM" != xosx],
    [AC_CHECK_LIB([readline], [main], [],
        [AC_MSG_ERROR([libreadline is required for profanity])])],
    [AC_CHECK_FILE([/usr/local/opt/readline/lib],
        [LIBS="-lreadline $LIBS"
            AM_CPPFLAGS="-I/usr/local/opt/readline/include $AM_CPPFLAGS"
            AM_LDFLAGS="-L/usr/local/opt/readline/lib $AM_LDFLAGS"
            AC_SUBST(AM_LDFLAGS)],
        [AC_MSG_ERROR([libreadline is required for profanity])])])

### Check for desktop notification support
### Linux/FreeBSD require libnotify
### Windows uses native OS calls
### OSX requires terminal-notifier

AS_IF([test "x$PLATFORM" = xosx],
        [AS_IF([test "x$enable_notifications" != xno],
            [NOTIFIER_PATH="no"
            AC_PATH_PROG(NOTIFIER_PATH, terminal-notifier, no)
            AS_IF([test "x$NOTIFIER_PATH" = xno],
                [AS_IF([test "x$enable_notifications" = xyes],
                    [AC_MSG_ERROR([terminal-notifier not found, required for desktop notifications.])],
                    [AC_MSG_NOTICE([Desktop notifications not supported.])])],
                [AC_DEFINE([HAVE_OSXNOTIFY], [1], [terminal notifier])])])],
    [test "x$PLATFORM" = xnix -o "x$PLATFORM" = xfreebsd],
        [AS_IF([test "x$enable_notifications" != xno],
            [PKG_CHECK_MODULES([libnotify], [libnotify],
                [AC_DEFINE([HAVE_LIBNOTIFY], [1], [libnotify module])],
                [AS_IF([test "x$enable_notifications" = xyes],
                    [AC_MSG_ERROR([libnotify is required but does not exist])],
                    [AC_MSG_NOTICE([libnotify support will be disabled])])])])])

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

AM_CONDITIONAL([BUILD_PGP], [false])
if test "x$enable_pgp" != xno; then
    AC_CHECK_LIB([gpgme], [main],
        [AM_CONDITIONAL([BUILD_PGP], [true])
         AC_DEFINE([HAVE_LIBGPGME], [1], [Have libgpgme])
         AC_PATH_PROG([GPGME_CONFIG], [gpgme-config], ["failed"])
         AS_IF([test "x$GPGME_CONFIG" = xfailed],
            [LIBS="-lgpgme $LIBS"],
            [LIBS="`$GPGME_CONFIG --libs` $LIBS" AM_CPPFLAGS="`$GPGME_CONFIG --cflags` $AM_CPPFLAGS"])],
        [AS_IF([test "x$enable_pgp" = xyes],
            [AC_MSG_ERROR([libgpgme is required for pgp support])],
            [AC_MSG_NOTICE([libgpgme not found, pgp support not enabled])])])
fi

AM_CONDITIONAL([BUILD_OTR], [false])
AM_CONDITIONAL([BUILD_OTR3], [false])
AM_CONDITIONAL([BUILD_OTR4], [false])
if test "x$enable_otr" != xno; then
    AC_SEARCH_LIBS([otrl_init], [otr],
        [AC_COMPILE_IFELSE(
            [AC_LANG_PROGRAM([[
            #include <libotr/version.h>
            ]],[[
            #if OTRL_VERSION_MAJOR == 4
            // OK
            #else
            # assume version 3
            #endif
            ]])],
            [AM_CONDITIONAL([BUILD_OTR], [true]) AM_CONDITIONAL([BUILD_OTR4], [true]) AC_DEFINE([HAVE_LIBOTR], [1], [Have libotr])],
            [AM_CONDITIONAL([BUILD_OTR], [true]) AM_CONDITIONAL([BUILD_OTR3], [true]) AC_DEFINE([HAVE_LIBOTR], [1], [Have libotr])])],
        [AS_IF([test "x$enable_otr" = xyes],
            [AC_MSG_ERROR([libotr is required for otr encryption support])],
            [AC_MSG_NOTICE([libotr not found, otr encryption support not enabled])])])
fi

AS_IF([test "x$with_themes" = xno],
    [THEMES_INSTALL="false"],
    [THEMES_INSTALL="true"])
AS_IF([test "x$with_themes" = xno -o "x$with_themes" = xyes -o "x$with_themes" = x],
    [THEMES_PATH='${pkgdatadir}/themes'],
    [THEMES_PATH="$with_themes"])
AC_SUBST(THEMES_PATH)
AM_CONDITIONAL([THEMES_INSTALL], "$THEMES_INSTALL")

ICONS_PATH='${pkgdatadir}/icons'
AC_SUBST(ICONS_PATH)

### cmocka is required only for tests, profanity shouldn't be linked with it
### TODO: pass cmocka_CFLAGS and cmocka_LIBS to Makefile.am
PKG_CHECK_MODULES([cmocka], [cmocka], [],
    [AC_MSG_NOTICE([cmocka is not found, will not be able to run tests])])

AM_CONDITIONAL([HAVE_STABBER], [false])
AC_CHECK_LIB([stabber], [stbbr_start], [AM_CONDITIONAL([HAVE_STABBER], [true])],
    [AC_MSG_NOTICE([libstabber not found, will not be able to run functional tests])])
AM_CONDITIONAL([HAVE_EXPECT], [false])
AC_CHECK_LIB([expect], [main], [AM_CONDITIONAL([HAVE_EXPECT], [true])],
    [AC_MSG_NOTICE([libexpect not found, will not be able to run functional tests])])

### Check for ncursesw/ncurses.h first, Arch linux uses ncurses.h for ncursesw
AC_CHECK_HEADERS([ncursesw/ncurses.h], [], [])
AC_CHECK_HEADERS([ncurses.h], [], [])

### Default parameters
AM_CFLAGS="-Wall -Wno-deprecated-declarations"
AS_IF([test "x$PACKAGE_STATUS" = xdevelopment],
    [AM_CFLAGS="$AM_CFLAGS -Wunused -Werror"])
AS_IF([test "x$PLATFORM" = xosx],
    [AM_CFLAGS="$AM_CFLAGS -Qunused-arguments"])
AM_LDFLAGS="$AM_LDFLAGS -export-dynamic"
AM_CPPFLAGS="$AM_CPPFLAGS $glib_CFLAGS $curl_CFLAGS $libnotify_CFLAGS $PYTHON_CPPFLAGS ${GTK_CFLAGS}"
AM_CPPFLAGS="$AM_CPPFLAGS -DTHEMES_PATH=\"\\\"$THEMES_PATH\\\"\" -DICONS_PATH=\"\\\"$ICONS_PATH\\\"\""
LIBS="$glib_LIBS $curl_LIBS $libnotify_LIBS $PYTHON_LIBS $PYTHON_LDFLAGS ${GTK_LIBS} -lgio-2.0 $LIBS"

AC_SUBST(AM_LDFLAGS)
AC_SUBST(AM_CFLAGS)
AC_SUBST(AM_CPPFLAGS)

### Checks for library functions.
AC_CHECK_FUNCS([atexit memset strdup strstr])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

echo ""
echo "PLATFORM       : $host_os"
echo "PACKAGE_STATUS : $PACKAGE_STATUS"
echo "AM_CFLAGS      : $AM_CFLAGS"
echo "AM_CPPFLAGS    : $AM_CPPFLAGS"
echo "AM_LDFLAGS     : $AM_LDFLAGS"
echo "LIBS           : $LIBS"
echo "Install themes : $THEMES_INSTALL"
echo "Themes path    : $THEMES_PATH"
echo "Icons path     : $ICONS_PATH"
echo ""
echo "Now you can run \`make' to build profanity"
