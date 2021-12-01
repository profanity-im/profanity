project('profanity',
        'c',
        license: 'GPL3+',
        default_options:  ['c_std=gnu99'],
        version: '0.12.0',
        meson_version: '>= 0.54.0')


cc = meson.get_compiler('c')
conf = configuration_data()


# general dependencies
glib_dep = dependency('glib-2.0', version: '>= 2.62.0')
gio_dep = dependency('gio-2.0')
curl_dep = dependency('libcurl', version: '>= 7.62.0')
sqlite_dep = dependency('sqlite3', version: '>= 3.22.0')
thread_dep = dependency('threads') # pthread
thread_dep = dependency('readline')
m_dep = cc.find_library('m', required : true)


# this will detect ncursesw etc (-I / -l flags)
curses_dep = dependency('curses', required: false)
# get ncurses definitions
check_headers = [
  ['ncursesw/menu.h', 'HAVE_NCURSESW_MENU_H'],
  ['ncurses/menu.h', 'HAVE_NCURSES_MENU_H'],
  ['menu.h', 'HAVE_MENU_H'],
  ['ncursesw/curses.h', 'HAVE_NCURSESW_CURSES_H'],
  ['ncursesw.h', 'HAVE_NCURSESW_H'],
  ['ncurses/curses.h', 'HAVE_NCURSES_CURSES_H'],
  ['ncurses.h', 'HAVE_NCURSES_H'],
  ['curses.h', 'HAVE_CURSES_H'],
]

foreach h : check_headers
  if cc.has_header(h.get(0))
    conf.set(h.get(1), 1)
  endif
endforeach


libstrophe_dep = dependency('libstrophe', version: '>= 0.11.0')
conf.set('HAVE_LIBSTROPHE', 1)


# optional dependencies
notify_dep = dependency('libnotify', required : get_option('notifications'))
if notify_dep.found()
  conf.set_quoted('HAVE_LIBNOTIFY', '1')
endif
# TODO: check for otr3 and otr4
otr_dep = dependency('libotr', version: '>= 4.0', required : get_option('otr'))
if otr_dep.found()
  conf.set_quoted('BUILD_OTR', '1')
endif
pgp_dep = dependency('gpgme', required : get_option('pgp'))
if pgp_dep.found()
  conf.set_quoted('HAVE_LIBGPGME', '1')
endif
# TODO: HAVE_LIBSIGNAL_LT_2_3_2
omemo_libsignal_dep = dependency('libsignal-protocol-c', version: '>= 2.3.2', required : get_option('pgp'))
omemo_gcrypt_dep = dependency('libsignal-protocol-c', version: '>= 1.7.0', required : get_option('pgp'))
if omemo_libsignal_dep.found() and omemo_gcrypt_dep.found()
  conf.set_quoted('BUILD_OMEMO', '1')
endif
# TODO: Xss not detected?
screensaver_xss_dep = dependency('xscrnsaver', required : get_option('xscreensaver'))
screensaver_x11_dep = dependency('X11', required : get_option('xscreensaver'))
if screensaver_xss_dep.found() and screensaver_x11_dep.found()
  conf.set_quoted('HAVE_LIBXSS', '1')
endif
icons_clipboard_gtk_dep = dependency('gtk+-3.0', version: '>= 3.24.0', required : get_option('icons-and-clipboard'))
if icons_clipboard_gtk_dep.found()
  conf.set_quoted('HAVE_GTK', '1')
endif


# TODO: set ICONS_PATH

conf.set_quoted('PACKAGE_VERSION', meson.project_version())
conf.set_quoted('PACKAGE_STATUS', 'development')
# TODO: PROF_GIT_BRANCH, PROF_GIT_REVISION, PACKAGE_STATUS,


# change later:
conf.set_quoted('GLOBAL_PYTHON_PLUGINS_PATH', '')
conf.set_quoted('GLOBAL_C_PLUGINS_PATH', '')


config_h = configure_file(output : 'config.h', configuration : conf)

subdir('src')

# TODO: output build info

profanity = executable('profanity',
            sources,
            dependencies : [ glib_dep,
                             gio_dep,
                             curl_dep,
                             sqlite_dep,
                             thread_dep,
                             m_dep,
                             curses_dep,
                             notify_dep,
                             otr_dep,
                             otr_internal_dep,
                             pgp_dep,
                             omemo_libsignal_dep,
                             omemo_gcrypt_dep,
                             screensaver_xss_dep,
                             screensaver_x11_dep,
                             icons_clipboard_gtk_dep,
                             libstrophe_dep ],
            include_directories : incdir,
            install : true)
