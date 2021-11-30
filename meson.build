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


conf.set_quoted('PACKAGE_VERSION', meson.project_version())
conf.set_quoted('PACKAGE_STATUS', 'development')
# TODO: PROF_GIT_BRANCH, PROF_GIT_REVISION, PACKAGE_STATUS,


# change later:
conf.set_quoted('GLOBAL_PYTHON_PLUGINS_PATH', '')
conf.set_quoted('GLOBAL_C_PLUGINS_PATH', '')


config_h = configure_file(output : 'config.h', configuration : conf)

subdir('src')

profanity = executable('profanity',
            core_sources_c,
            dependencies : [ glib_dep,
                             gio_dep,
                             curl_dep,
                             sqlite_dep,
                             thread_dep,
                             m_dep,
                             curses_dep,
                             notify_dep,
                             libstrophe_dep ],
            include_directories : incdir,
            install : true)
