# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([profanity],[0.12.1],[jubalh@iodoru.org],[profanity],[https://profanity-im.github.io/])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADERS([src/config.h])

AC_PROG_CC
LT_INIT
AC_SYS_LARGEFILE

## Determine platform
AC_CANONICAL_TARGET
PLATFORM="unknown"
AS_CASE([$target_os],
    [freebsd*], [PLATFORM="freebsd"],
    [netbsd*], [PLATFORM="netbsd"],
    [openbsd*], [PLATFORM="openbsd"],
    [darwin*], [PLATFORM="osx"],
    [cygwin], [PLATFORM="cygwin"],
    [PLATFORM="nix"])

AM_INIT_AUTOMAKE([foreign subdir-objects])

PACKAGE_STATUS="development"

## Get git branch and revision if in development
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

## Environment variables
AC_ARG_VAR([PYTHON_FRAMEWORK], [Set base directory for Python Framework])

## Options
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
AC_ARG_ENABLE([omemo],
    [AS_HELP_STRING([--enable-omemo], [enable OMEMO encryption])])
AC_ARG_WITH([xscreensaver],
    [AS_HELP_STRING([--with-xscreensaver], [use libXScrnSaver to determine idle time])])
AC_ARG_WITH([themes],
    [AS_HELP_STRING([--with-themes[[=PATH]]], [install themes (default yes)])])
AC_ARG_ENABLE([icons-and-clipboard],
    [AS_HELP_STRING([--enable-icons-and-clipboard], [enable GTK tray icons and clipboard paste support])])

# Required dependencies

AC_CHECK_FUNCS([atexit memset strdup strstr])

PKG_CHECK_MODULES([glib], [glib-2.0 >= 2.62.0], [],
    [AC_MSG_ERROR([glib 2.62.0 or higher is required])])
PKG_CHECK_MODULES([gio], [gio-2.0], [],
    [AC_MSG_ERROR([libgio-2.0 from glib-2.0 is required])])

AC_SEARCH_LIBS([fmod], [m], [],
   [AC_MSG_ERROR([math.h is required])], [])

PKG_CHECK_MODULES([curl], [libcurl >= 7.62.0], [],
    [AC_CHECK_LIB([curl], [main], [],
        [AC_MSG_ERROR([libcurl 7.62.0 or higher is required])])])

PKG_CHECK_MODULES([SQLITE], [sqlite3 >= 3.22.0], [],
    [AC_MSG_ERROR([sqlite3 3.22.0 or higher is required])])

ACX_PTHREAD([], [AC_MSG_ERROR([pthread is required])])
AS_IF([test "x$PTHREAD_CC" != x], [ CC="$PTHREAD_CC" ])

### plugins

## python
if test "x$enable_plugins" = xno; then
    AM_CONDITIONAL([BUILD_PYTHON_API], [false])
elif test "x$enable_python_plugins" != xno; then
    AS_IF([test "x$PLATFORM" = xosx], [
        AS_IF([test "x$PYTHON_FRAMEWORK" = x], [ PYTHON_FRAMEWORK="/Library/Frameworks/Python.framework" ])
        AC_MSG_NOTICE([Symlinking Python.framework to $PYTHON_FRAMEWORK])
        rm -f Python.framework
        ln -s $PYTHON_FRAMEWORK Python.framework ])
    PKG_CHECK_MODULES([python], [python-embed], [PYTHON_CONFIG_EXISTS=yes], [PYTHON_CONFIG_EXISTS=no])
    PKG_CHECK_MODULES([python], [python3-embed], [PYTHON3_CONFIG_EXISTS=yes; AC_DEFINE(PY_IS_PYTHON3, [1], [Is Python version 3])], [PYTHON3_CONFIG_EXISTS=no])
    if test "$PYTHON_CONFIG_EXISTS" = "yes" || test "$PYTHON3_CONFIG_EXISTS" = "yes"; then
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
    # set path to look for global python plugins
    GLOBAL_PYTHON_PLUGINS_PATH='${pkgdatadir}/plugins'
    AC_SUBST(GLOBAL_PYTHON_PLUGINS_PATH)
else
    AM_CONDITIONAL([BUILD_PYTHON_API], [false])
fi

## C
if test "x$PLATFORM" = xcygwin; then
    AM_CONDITIONAL([BUILD_C_API], [false])
else
    LT_INIT
    if test "x$enable_plugins" = xno; then
        AM_CONDITIONAL([BUILD_C_API], [false])
    elif test "x$enable_c_plugins" != xno; then
        # libdl doesn't exist as a separate library in the BSDs and is
        # provided in the standard libraries.
        AS_IF([test "x$PLATFORM" = xopenbsd -o "x$PLATFORM" = xfreebsd -o "x$PLATFORM" = xnetbsd],
            [AM_CONDITIONAL([BUILD_C_API], [true]) AC_DEFINE([HAVE_C], [1], [C support])],
            [AC_CHECK_LIB([dl], [main],
                [AM_CONDITIONAL([BUILD_C_API], [true]) LIBS="$LIBS -ldl" AC_DEFINE([HAVE_C], [1], [C support])],
                [AS_IF(
                    [test "x$enable_c_plugins" = xyes],
                        [AC_MSG_ERROR([dl library needed to run C plugins])],
                    [AM_CONDITIONAL([BUILD_C_API], [false])])
                ])])
        # set path to look for global c plugins
        GLOBAL_C_PLUGINS_PATH='${pkglibdir}/plugins'
        AC_SUBST(GLOBAL_C_PLUGINS_PATH)
    else
        AM_CONDITIONAL([BUILD_C_API], [false])
    fi
fi

## Check for libstrophe
PKG_CHECK_MODULES([libstrophe], [libstrophe >= 0.11.0],
    [LIBS="$libstrophe_LIBS $LIBS" CFLAGS="$CFLAGS $libstrophe_CFLAGS"])

AC_MSG_CHECKING([whether libstrophe works])
AC_LINK_IFELSE([AC_LANG_SOURCE([[
    #include <strophe.h>

    int main() {
        xmpp_initialize();
        return 0;
    }
    ]])],
    [AC_MSG_RESULT([yes])],
    [AC_MSG_ERROR([libstrophe is broken, check config.log for details])])

## Check for curses library
PKG_CHECK_MODULES([ncursesw], [ncursesw],
    [NCURSES_CFLAGS="$ncursesw_CFLAGS"; NCURSES_LIBS="$ncursesw_LIBS"; CURSES="ncursesw"],
    [PKG_CHECK_MODULES([ncurses], [ncurses],
        [NCURSES_CFLAGS="$ncurses_CFLAGS"; NCURSES_LIBS="$ncurses_LIBS"; CURSES="ncurses"],
        [AC_CHECK_LIB([ncursesw], [main], [],
            [AC_CHECK_LIB([ncurses], [main], [],
                [AC_CHECK_LIB([curses], [main],
                    [LIBS="$LIBS -lcurses"; CURSES="curses"],
                    [AC_MSG_ERROR([ncurses or curses is required for profanity])])])])])])
AM_CFLAGS="$AM_CFLAGS $NCURSES_CFLAGS"
LIBS="$NCURSES_LIBS $LIBS"

## Check wide characters support in curses library
CFLAGS_RESTORE="$CFLAGS"
CFLAGS="$CFLAGS $NCURSES_CFLAGS"
AC_CACHE_CHECK([for wget_wch support in $CURSES], ncurses_cv_wget_wch,
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

## Check for ncursesw/ncurses.h first, Arch linux uses ncurses.h for ncursesw
AC_CHECK_HEADERS([ncursesw/ncurses.h], [], [])
AC_CHECK_HEADERS([ncurses.h], [], [])
AC_CHECK_HEADERS([curses.h], [], [])

# Check for readline
AS_IF([test "x$PLATFORM" = xosx],
        [AC_PATH_PROG([BREW], [brew], ["failed"],
            [$PATH:/opt/homebrew/bin:/usr/local/bin])
        AS_IF([test "x$BREW" = xfailed],
            [AC_CHECK_FILE([/opt/local/lib/libreadline.dylib],
                [READLINE_PREFIX="/opt/local"],
                [READLINE_PREFIX="/usr/local"])],
            [READLINE_PREFIX="`$BREW --prefix readline`"])])

AS_IF([test "x$PLATFORM" = xosx],
    [AC_CHECK_FILE([$READLINE_PREFIX/lib/libreadline.dylib],
        [LIBS="-lreadline $LIBS"
            AM_CPPFLAGS="-I$READLINE_PREFIX/include $AM_CPPFLAGS"
            AM_LDFLAGS="-L$READLINE_PREFIX/lib $AM_LDFLAGS"
            AC_SUBST(AM_LDFLAGS)],
        [AC_MSG_ERROR([libreadline is required for profanity. Install it with Homebrew, MacPorts, or manually into /usr/local])])],

      [test "x$PLATFORM" = xopenbsd],
      [AC_CHECK_FILE([/usr/local/include/ereadline],
          [LIBS="-lereadline $LIBS"
              AM_CPPFLAGS="-I/usr/local/include/ereadline $AM_CFLAGS"
              AM_LDFLAGS="-L/usr/local/lib $AM_LDFLAGS"
              AC_SUBST(AM_LDFLAGS)])],

    [AC_CHECK_LIB([readline], [main], [],
        [AC_MSG_ERROR([libreadline is required for profanity])])])

## Check for optional dependencies depending on feature flags

dnl feature: icons-and-clipboard
AS_IF([test "x$enable_icons_and_clipboard" != xno],
    [PKG_CHECK_MODULES([GTK], [gtk+-3.0 >= 3.24.0],
        [AC_DEFINE([HAVE_GTK], [1], [libgtk module])],
        [AS_IF([test "x$enable_icons_and_clipboard" = xyes],
               [PKG_CHECK_MODULES([GTK], [gtk+-2.0 >= 2.24.10],
                                  [AC_DEFINE([HAVE_GTK], [1], [libgtk module])],
                                  [AC_MSG_ERROR([gtk+-2.0 >= 2.24.10 or gtk+-3.0 >= 3.24.0 is required for icons and clipboard])],
                                  [AC_MSG_NOTICE([gtk+-3.0/gtk+2.0 not found, icons and clipboard not enabled])])])])])

ICONS_PATH='${pkgdatadir}/icons'
AC_SUBST(ICONS_PATH)

dnl feature: notifications

## Check for desktop notification support
## Linux/FreeBSD require libnotify
## Windows uses native OS calls
## OSX requires terminal-notifier

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

dnl feature: xscreensaver
AS_IF([test "x$enable_xscreensaver" != xno],
    [PKG_CHECK_MODULES([xscrnsaver], [xscrnsaver],
        [AC_MSG_NOTICE([xscreensaver support is enabled])],
        [AS_IF([test "x$enable_xscreensaver" = xyes],
            [AC_MSG_ERROR([xscreensaver is required but does not exist])],
            [AC_MSG_NOTICE([xscreensaver support is disabled])])])])

dnl feature: pgp
AM_CONDITIONAL([BUILD_PGP], [false])
if test "x$enable_pgp" != xno; then
    AC_CHECK_LIB([gpgme], [main],
        [AM_CONDITIONAL([BUILD_PGP], [true])
         AC_DEFINE([HAVE_LIBGPGME], [1], [Have libgpgme])
         AC_PATH_PROG([GPGME_CONFIG], [gpgme-config], ["failed"])
         AS_IF([test "x$GPGME_CONFIG" = xfailed],
            [LIBS="-lgpgme $LIBS"],
            [LIBS="`$GPGME_CONFIG --libs` $LIBS" AM_CFLAGS="`$GPGME_CONFIG --cflags` $AM_CFLAGS"])],
        [AS_IF([test "x$enable_pgp" = xyes],
            [AC_MSG_ERROR([libgpgme is required for pgp support])],
            [AC_MSG_NOTICE([libgpgme not found, pgp support not enabled])])])
fi

dnl feature: otr
AM_CONDITIONAL([BUILD_OTR], [false])
if test "x$enable_otr" != xno; then
    AM_CONDITIONAL([BUILD_OTR], [true])

    PKG_CHECK_MODULES([libotr], [libotr >= 4.0],
        [LIBS="$libotr_LIBS $LIBS" CFLAGS="$libotr_CFLAGS $cflags"],
        [AM_CONDITIONAL([BUILD_OTR], [false])
         AS_IF([test "x$enable_otr" = xyes],
               [AC_MSG_ERROR([libotr >= 4.0 is required for OTR support])],
               [AC_MSG_NOTICE([libotr >= 4.0 not found, OTR support not enabled])])])

   AM_COND_IF([BUILD_OTR], [AC_DEFINE([HAVE_LIBOTR], [1], [Have libotr])])
fi

dnl feature: omemo
AM_CONDITIONAL([BUILD_OMEMO], [false])
if test "x$enable_omemo" != xno; then
   AM_CONDITIONAL([BUILD_OMEMO], [true])
    PKG_CHECK_MODULES([libsignal], [libsignal-protocol-c >= 2.3.2],
        [LIBS="-lsignal-protocol-c $LIBS"],
        [AC_MSG_NOTICE([libsignal-protocol-c >= 2.3.2 not found, OMEMO support not enabled])])

    AC_CHECK_LIB([gcrypt], [gcry_md_extract],
        [LIBS="-lgcrypt $LIBS"],
        [AM_CONDITIONAL([BUILD_OMEMO], [false])
         AS_IF([test "x$enable_omemo" = xyes],
               [AC_MSG_ERROR([gcrypt >= 1.7.0 is required for OMEMO support])],
               [AC_MSG_NOTICE([gcrypt >= 1.7.0 not found, OMEMO support not enabled])])])

   AM_COND_IF([BUILD_OMEMO], [AC_DEFINE([HAVE_OMEMO], [1], [Have OMEMO])])
fi

dnl feature: themes
AS_IF([test "x$with_themes" = xno],
    [THEMES_INSTALL="false"],
    [THEMES_INSTALL="true"])
AS_IF([test "x$with_themes" = xno -o "x$with_themes" = xyes -o "x$with_themes" = x],
    [THEMES_PATH='${pkgdatadir}/themes'],
    [THEMES_PATH="$with_themes"])
AC_SUBST(THEMES_PATH)
AM_CONDITIONAL([THEMES_INSTALL], "$THEMES_INSTALL")

## Tests

# cmocka is required only for tests, profanity shouldn't be linked with it
# TODO: pass cmocka_CFLAGS and cmocka_LIBS to Makefile.am
PKG_CHECK_MODULES([cmocka], [cmocka], [],
    [AC_MSG_NOTICE([cmocka is not found, will not be able to run tests])])

AM_CONDITIONAL([HAVE_STABBER], [false])
AC_CHECK_LIB([stabber], [stbbr_start], [AM_CONDITIONAL([HAVE_STABBER], [true])],
    [AC_MSG_NOTICE([libstabber not found, will not be able to run functional tests])])
AM_CONDITIONAL([HAVE_EXPECT], [false])
AC_CHECK_LIB([expect], [exp_expectl], [AM_CONDITIONAL([HAVE_EXPECT], [true])],
    [AC_MSG_NOTICE([libexpect not found, will not be able to run functional tests])])

## Default parameters
AM_CFLAGS="$AM_CFLAGS -Wall -Wno-deprecated-declarations -std=gnu99"
AM_CFLAGS="$AM_CFLAGS -Wall -Wno-deprecated-declarations -std=gnu99 -ggdb3"
AM_LDFLAGS="$AM_LDFLAGS -export-dynamic"

AS_IF([test "x$PACKAGE_STATUS" = xdevelopment],
    [AM_CFLAGS="$AM_CFLAGS -Wunused -Werror"])
AS_IF([test "x$PLATFORM" = xosx],
    [AM_CFLAGS="$AM_CFLAGS -Qunused-arguments"])

AM_CFLAGS="$AM_CFLAGS $PTHREAD_CFLAGS $glib_CFLAGS $gio_CFLAGS $curl_CFLAGS ${SQLITE_CFLAGS} $libstrophe_CFLAGS"
AM_CFLAGS="$AM_CFLAGS $libnotify_CFLAGS ${GTK_CFLAGS} $python_CFLAGS"
AM_CFLAGS="$AM_CFLAGS -DTHEMES_PATH=\"\\\"$THEMES_PATH\\\"\" -DICONS_PATH=\"\\\"$ICONS_PATH\\\"\" -DGLOBAL_PYTHON_PLUGINS_PATH=\"\\\"$GLOBAL_PYTHON_PLUGINS_PATH\\\"\" -DGLOBAL_C_PLUGINS_PATH=\"\\\"$GLOBAL_C_PLUGINS_PATH\\\"\""
LIBS="$glib_LIBS $gio_LIBS $PTHREAD_LIBS $curl_LIBS $libnotify_LIBS $python_LIBS ${GTK_LIBS} ${SQLITE_LIBS} $LIBS"

AC_SUBST(AM_LDFLAGS)
AC_SUBST(AM_CFLAGS)
AC_SUBST(AM_CPPFLAGS)

## Output
AC_CONFIG_FILES([Makefile])
AC_OUTPUT

AC_MSG_NOTICE([Summary of build options:
PLATFORM                   : $target_os
PACKAGE_STATUS             : $PACKAGE_STATUS
LIBS                       : $LIBS
Install themes             : $THEMES_INSTALL
Themes path                : $THEMES_PATH
Icons path                 : $ICONS_PATH
Global Python plugins path : $GLOBAL_PYTHON_PLUGINS_PATH
Global C plugins path      : $GLOBAL_C_PLUGINS_PATH
CFLAGS                     : $AM_CFLAGS
CPPFLAGS                   : $AM_CPPFLAGS
LDFLAGS                    : $AM_LDFLAGS
LIBS                       : $LIBS

Run `make' to build profanity])

dnl vim: expandtab:ts=4:sts=4:sw=4
