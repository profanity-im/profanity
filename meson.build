project('profanity', 'c',
  version: '0.16.0',
  license: 'GPL-3.0-or-later',
  meson_version: '>= 0.56.0',
  default_options: [
    'c_std=gnu99',
    'warning_level=2',
  ]
)

# Determine platform
host_system = host_machine.system()
platform_map = {
  'freebsd': 'freebsd',
  'netbsd':  'netbsd',
  'openbsd': 'openbsd',
  'darwin':  'osx',
  'cygwin':  'cygwin',
}
platform = platform_map.get(host_system, 'nix')

# Configuration data
conf_data = configuration_data()
conf_data.set_quoted('PACKAGE_NAME', meson.project_name())
conf_data.set_quoted('PACKAGE_VERSION', meson.project_version())
conf_data.set_quoted('PACKAGE_BUGREPORT', 'jubalh@iodoru.org')
conf_data.set_quoted('PACKAGE_URL', 'https://profanity-im.github.io/')

# Possible options from meson:
# debug, debugoptimized, release, plain
# Build type configuration
is_release = get_option('buildtype') == 'release'
is_debug = get_option('buildtype') in ['debug', 'debugoptimized']

conf_data.set_quoted('PACKAGE_STATUS', is_release ? 'release' : 'development')

if platform == 'cygwin'
  conf_data.set('PLATFORM_CYGWIN', 1)
endif

if platform == 'osx'
  conf_data.set('PLATFORM_OSX', 1)
endif

# Get git version if in development
if is_debug
  git = find_program('git', required: false)
  if git.found()
    conf_data.set('HAVE_GIT_VERSION', 1)
  endif
endif

# Compiler setup
cc = meson.get_compiler('c')

# Common compiler flags
add_project_arguments([
  '-Wno-deprecated-declarations',
  '-Wno-unused-parameter',
], language: 'c')

if is_debug
  add_project_arguments([
    '-ggdb3',
    '-Wunused',
    '-Wall',
    '-Wextra',
    '-fstack-protector-strong',
    '-D_FORTIFY_SOURCE=2',
    '-Og',
    '-Wpointer-arith'
  ], language: 'c')
  if cc.get_id() == 'gcc'
     add_project_arguments('-fanalyzer', language: 'c')
  endif
endif

if cc.get_id() == 'clang'
  add_project_arguments('-Qunused-arguments', language: 'c')
endif

if platform == 'osx'
  # Check for homebrew paths on Apple Silicon
  if run_command('test', '-d', '/opt/homebrew/include', check: false).returncode() == 0
    add_project_arguments('-I/opt/homebrew/include', language: 'c')
    add_project_link_arguments('-L/opt/homebrew/lib', language: 'c')
  endif
endif

# Required dependencies
glib_dep = dependency('glib-2.0', version: '>= 2.62.0')
gio_dep = dependency('gio-2.0')
curl_dep = dependency('libcurl', version: '>= 7.62.0')
sqlite_dep = dependency('sqlite3', version: '>= 3.22.0')
thread_dep = dependency('threads')
math_dep = cc.find_library('m')
libstrophe_dep = dependency('libstrophe', version: '>= 0.12.3')

# Check for XMPP_CERT_PUBKEY_FINGERPRINT_SHA256 support
if cc.links('''
  #include <strophe.h>
  int main() {
    xmpp_tlscert_get_string(NULL, XMPP_CERT_PUBKEY_FINGERPRINT_SHA256);
    return 1;
  }
  ''', dependencies: libstrophe_dep)
  conf_data.set('HAVE_XMPP_CERT_PUBKEY_FINGERPRINT_SHA256', 1)
endif

# ncurses
curses_dep = dependency('ncursesw', required: false)
if not curses_dep.found()
  curses_dep = dependency('ncurses', required: false)
endif
if not curses_dep.found()
  curses_dep = cc.find_library('ncursesw', required: false)
endif
if not curses_dep.found()
  curses_dep = cc.find_library('ncurses', required: true)
endif

# Check for ncursesw/ncurses.h, Arch Linux uses ncurses.h for ncursesw
if cc.has_header('ncursesw/ncurses.h')
  conf_data.set('HAVE_NCURSESW_NCURSES_H', 1)
elif cc.has_header('ncurses.h')
  conf_data.set('HAVE_NCURSES_H', 1)
endif

# Readline
readline_dep = dependency('readline', required: false)

if not readline_dep.found() and platform == 'osx'
  brew = find_program('brew', required: false)
  if brew.found()
    prefix = run_command(brew, '--prefix', 'readline', check: false).stdout().strip()
    readline_dep = cc.find_library('readline', dirs: [prefix / 'lib'], required: true)
    add_project_arguments('-I' + prefix / 'include', language: 'c')
  endif
endif

if not readline_dep.found() and platform == 'openbsd'
  readline_dep = cc.find_library('ereadline', dirs: ['/usr/local/lib'], required: false)
endif

if not readline_dep.found()
  readline_dep = cc.find_library('readline', required: true)
endif

# Optional dependencies
have_osxnotify = false
libnotify_dep = disabler()
gtk_dep = disabler()
xscrnsaver_dep = []
python_dep = disabler()
dl_dep = disabler()
gpgme_dep = disabler()
libotr_dep = disabler()
gdk_pixbuf_dep = disabler()
libsignal_dep = disabler()
gcrypt_dep = disabler()
qrencode_dep = disabler()

# Build flags
build_python_api = false
build_c_api = false
build_pgp = false
build_otr = false
build_omemo = false

# Notifications
if get_option('notifications').enabled()
  if platform == 'osx'
    terminal_notifier = find_program('terminal-notifier', required: true)
    have_osxnotify = true
    conf_data.set('HAVE_OSXNOTIFY', 1)
  elif platform in ['nix', 'freebsd']
    libnotify_dep = dependency('libnotify', required: true)
    conf_data.set('HAVE_LIBNOTIFY', 1)
  else
    error('Notifications not supported on this platform')
  endif
endif

# GTK (for icons and clipboard)
if get_option('icons-and-clipboard').enabled()
  gtk_dep = dependency('gtk+-3.0', version: '>= 3.24.0', required: true)
  conf_data.set('HAVE_GTK', 1)
endif

# XScreenSaver
if get_option('xscreensaver').enabled()
  xscrnsaver_dep = [
    dependency('xscrnsaver', required: true),
    dependency('x11', required: true)
  ]
  conf_data.set('HAVE_LIBXSS', 1)
endif

# Python plugins
if get_option('python-plugins').enabled()
  python_dep = dependency('python3-embed', required: false)
  if not python_dep.found()
    python_dep = dependency('python-embed', required: true)
  else
    conf_data.set('PY_IS_PYTHON3', 1)
  endif
  
  build_python_api = true
  conf_data.set('HAVE_PYTHON', 1)
endif

# C plugins
if get_option('c-plugins').enabled()
  if platform == 'cygwin'
    error('C plugins are not supported on Cygwin')
  endif
  
  if platform not in ['openbsd', 'freebsd', 'netbsd']
    dl_dep = cc.find_library('dl', required: true)
  endif
  
  build_c_api = true
  conf_data.set('HAVE_C', 1)
endif

# PGP support
if get_option('pgp').enabled()
  gpgme_dep = dependency('gpgme', required: false)
  
  build_pgp = true
  conf_data.set('HAVE_LIBGPGME', 1)
endif

# OTR support
if get_option('otr').enabled()
  libotr_dep = dependency('libotr', version: '>= 4.0', required: true)
  build_otr = true
  conf_data.set('HAVE_LIBOTR', 1)
endif

# GDK Pixbuf (for avatar scaling)
if get_option('gdk-pixbuf').enabled()
  gdk_pixbuf_dep = dependency('gdk-pixbuf-2.0', version: '>= 2.4', required: true)
  conf_data.set('HAVE_PIXBUF', 1)
endif

# OMEMO support
if get_option('omemo').enabled()
  libsignal_dep = dependency('libsignal-protocol-c', version: '>= 2.3.2', required: true)
  gcrypt_dep = dependency('libgcrypt', version: '>= 1.7.0', required: true)
  build_omemo = true
  conf_data.set('HAVE_OMEMO', 1)
endif

# QR code support (for OMEMO)
if get_option('omemo-qrcode').enabled()
  qrencode_dep = dependency('libqrencode', required: true)
  conf_data.set('HAVE_QRENCODE', 1)
endif

# Set installation paths
themes_path = get_option('themes_path')
if themes_path == ''
  themes_path = get_option('datadir') / meson.project_name() / 'themes'
endif

icons_path = get_option('datadir') / meson.project_name() / 'icons'
global_python_plugins_path = get_option('datadir') / meson.project_name() / 'plugins'
global_c_plugins_path = get_option('libdir') / meson.project_name() / 'plugins'

conf_data.set_quoted('THEMES_PATH', get_option('prefix') / themes_path)
conf_data.set_quoted('ICONS_PATH', get_option('prefix') / icons_path)
conf_data.set_quoted('GLOBAL_PYTHON_PLUGINS_PATH', get_option('prefix') / global_python_plugins_path)
conf_data.set_quoted('GLOBAL_C_PLUGINS_PATH', get_option('prefix') / global_c_plugins_path)

# Check for required functions
foreach func : ['atexit', 'memset', 'strdup', 'strstr']
  if cc.has_function(func)
    conf_data.set('HAVE_' + func.to_upper(), 1)
  endif
endforeach

# Generate config.h
config_h = configure_file(
  output: 'config.h',
  configuration: conf_data,
)

# Build dependencies list
profanity_deps = [
  glib_dep,
  gio_dep,
  curl_dep,
  sqlite_dep,
  thread_dep,
  math_dep,
  libstrophe_dep,
  curses_dep,
  readline_dep,
]

# Add optional dependencies only if found
if libnotify_dep.found()
  profanity_deps += libnotify_dep
endif

if gtk_dep.found()
  profanity_deps += gtk_dep
endif

if xscrnsaver_dep.length() > 0
  profanity_deps += xscrnsaver_dep
endif

if python_dep.found()
  profanity_deps += python_dep
endif

if dl_dep.found()
  profanity_deps += dl_dep
endif

if gpgme_dep.found()
  profanity_deps += gpgme_dep
endif

if libotr_dep.found()
  profanity_deps += libotr_dep
endif

if gdk_pixbuf_dep.found()
  profanity_deps += gdk_pixbuf_dep
endif

if libsignal_dep.found()
  profanity_deps += libsignal_dep
endif

if gcrypt_dep.found()
  profanity_deps += gcrypt_dep
endif

if qrencode_dep.found()
  profanity_deps += qrencode_dep
endif

# Include directories
inc = include_directories('.', 'src')

# Core source files
core_sources = files(
  'src/xmpp/contact.c',
  'src/log.c',
  'src/common.c',
  'src/chatlog.c',
  'src/database.c',
  'src/profanity.c',
  'src/xmpp/chat_session.c',
  'src/xmpp/muc.c',
  'src/xmpp/jid.c',
  'src/xmpp/chat_state.c',
  'src/xmpp/resource.c',
  'src/xmpp/roster_list.c',
  'src/xmpp/capabilities.c',
  'src/xmpp/session.c',
  'src/xmpp/connection.c',
  'src/xmpp/iq.c',
  'src/xmpp/message.c',
  'src/xmpp/presence.c',
  'src/xmpp/stanza.c',
  'src/xmpp/roster.c',
  'src/xmpp/bookmark.c',
  'src/xmpp/blocking.c',
  'src/xmpp/form.c',
  'src/xmpp/avatar.c',
  'src/xmpp/ox.c',
  'src/xmpp/vcard.c',
  'src/event/common.c',
  'src/event/server_events.c',
  'src/event/client_events.c',
  'src/ui/window.c',
  'src/ui/core.c',
  'src/ui/titlebar.c',
  'src/ui/statusbar.c',
  'src/ui/inputwin.c',
  'src/ui/screen.c',
  'src/ui/console.c',
  'src/ui/notifier.c',
  'src/ui/window_list.c',
  'src/ui/rosterwin.c',
  'src/ui/occupantswin.c',
  'src/ui/buffer.c',
  'src/ui/chatwin.c',
  'src/ui/mucwin.c',
  'src/ui/privwin.c',
  'src/ui/confwin.c',
  'src/ui/xmlwin.c',
  'src/ui/vcardwin.c',
  'src/command/cmd_defs.c',
  'src/command/cmd_funcs.c',
  'src/command/cmd_ac.c',
  'src/tools/parser.c',
  'src/tools/http_common.c',
  'src/tools/http_upload.c',
  'src/tools/http_download.c',
  'src/tools/plugin_download.c',
  'src/tools/bookmark_ignore.c',
  'src/tools/autocomplete.c',
  'src/tools/clipboard.c',
  'src/tools/editor.c',
  'src/config/files.c',
  'src/config/conflists.c',
  'src/config/accounts.c',
  'src/config/tlscerts.c',
  'src/config/account.c',
  'src/config/preferences.c',
  'src/config/theme.c',
  'src/config/color.c',
  'src/config/scripts.c',
  'src/config/cafile.c',
  'src/plugins/plugins.c',
  'src/plugins/api.c',
  'src/plugins/callbacks.c',
  'src/plugins/autocompleters.c',
  'src/plugins/themes.c',
  'src/plugins/settings.c',
  'src/plugins/disco.c',
)

# Build the final source list
profanity_sources = core_sources

# Add conditional sources
if gtk_dep.found()
  profanity_sources += files('src/ui/tray.c')
endif

if build_python_api
  profanity_sources += files(
    'src/plugins/python_plugins.c',
    'src/plugins/python_api.c',
  )
endif

if build_c_api
  profanity_sources += files(
    'src/plugins/c_plugins.c',
    'src/plugins/c_api.c',
  )
endif

if build_pgp
  profanity_sources += files(
    'src/pgp/gpg.c',
    'src/pgp/ox.c',
  )
endif

if build_otr
  profanity_sources += files(
    'src/otr/otrlibv4.c',
    'src/otr/otr.c',
  )
endif

if build_omemo
  profanity_sources += files(
    'src/omemo/omemo.c',
    'src/omemo/crypto.c',
    'src/omemo/store.c',
    'src/xmpp/omemo.c',
    'src/tools/aesgcm_download.c',
  )
endif

# Generate git version header if in development
if is_debug
  git_branch = run_command('git', 'rev-parse', '--symbolic-full-name', '--abbrev-ref', 'HEAD', 
                           check: false).stdout().strip()
  git_revision = run_command('git', 'log', '--pretty=format:%h', '-n', '1',
                             check: false).stdout().strip()
  
  git_version_conf = configuration_data()
  git_version_conf.set('PROF_GIT_BRANCH', '"' + git_branch + '"')
  git_version_conf.set('PROF_GIT_REVISION', '"' + git_revision + '"')
  
  configure_file(
    input: 'src/gitversion.h.in',
    output: 'gitversion.h',
    configuration: git_version_conf,
  )
endif

# Build the executable
profanity_exe = executable(
  'profanity',
  profanity_sources,
  files('src/main.c'),
  config_h,
  dependencies: profanity_deps,
  include_directories: inc,
  install: true,
  export_dynamic: true,
)

# Build libprofanity shared library for C plugins
if build_c_api
  libprofanity = shared_library(
    'profanity',
    'src/plugins/profapi.c',
    dependencies: profanity_deps,
    include_directories: inc,
    install: true,
    version: '0.0.0',
  )
  
  install_headers('src/plugins/profapi.h')
endif

# Install themes if enabled
if get_option('install_themes')
  install_subdir('themes',
    install_dir: get_option('datadir') / meson.project_name(),
    strip_directory: false,
  )
endif

# Install icons
install_subdir('icons',
  install_dir: get_option('datadir') / meson.project_name(),
  strip_directory: false,
)

# Install man pages
man_pages = run_command('find', 'docs', '-name', 'profanity*.1', '-type', 'f',
                        check: true).stdout().strip().split('\n')

if man_pages.length() > 0 and man_pages[0] != ''
  install_man(man_pages)
endif

# Install example config and theme template
install_data(
  'profrc.example',
  'theme_template',
  install_dir: get_option('datadir') / 'doc' / meson.project_name(),
)

# Tests
if get_option('tests')
  cmocka_dep = dependency('cmocka', required: false)
  if cmocka_dep.found()
    unittest_sources = files(
      'src/xmpp/contact.c',
      'src/common.c',
      'src/profanity.c',
      'src/xmpp/chat_session.c',
      'src/xmpp/muc.c',
      'src/xmpp/jid.c',
      'src/xmpp/resource.c',
      'src/xmpp/chat_state.c',
      'src/xmpp/roster_list.c',
      'src/xmpp/form.c',
      'src/command/cmd_defs.c',
      'src/command/cmd_funcs.c',
      'src/command/cmd_ac.c',
      'src/tools/parser.c',
      'src/tools/autocomplete.c',
      'src/tools/clipboard.c',
      'src/tools/editor.c',
      'src/tools/bookmark_ignore.c',
      'src/config/account.c',
      'src/config/files.c',
      'src/config/tlscerts.c',
      'src/config/preferences.c',
      'src/config/theme.c',
      'src/config/color.c',
      'src/config/scripts.c',
      'src/config/conflists.c',
      'src/plugins/plugins.c',
      'src/plugins/api.c',
      'src/plugins/callbacks.c',
      'src/plugins/autocompleters.c',
      'src/plugins/themes.c',
      'src/plugins/settings.c',
      'src/plugins/disco.c',
      'src/ui/window_list.c',
      'src/event/common.c',
      'src/event/server_events.c',
      'src/event/client_events.c',
      'src/ui/tray.c',
      'tests/unittests/xmpp/stub_vcard.c',
      'tests/unittests/xmpp/stub_avatar.c',
      'tests/unittests/xmpp/stub_ox.c',
      'tests/unittests/xmpp/stub_xmpp.c',
      'tests/unittests/xmpp/stub_message.c',
      'tests/unittests/ui/stub_ui.c',
      'tests/unittests/ui/stub_vcardwin.c',
      'tests/unittests/log/stub_log.c',
      'tests/unittests/chatlog/stub_chatlog.c',
      'tests/unittests/database/stub_database.c',
      'tests/unittests/config/stub_accounts.c',
      'tests/unittests/config/stub_cafile.c',
      'tests/unittests/tools/stub_http_upload.c',
      'tests/unittests/tools/stub_http_download.c',
      'tests/unittests/tools/stub_aesgcm_download.c',
      'tests/unittests/tools/stub_plugin_download.c',
      'tests/unittests/helpers.c',
      'tests/unittests/test_form.c',
      'tests/unittests/test_common.c',
      'tests/unittests/test_autocomplete.c',
      'tests/unittests/test_jid.c',
      'tests/unittests/test_parser.c',
      'tests/unittests/test_roster_list.c',
      'tests/unittests/test_chat_session.c',
      'tests/unittests/test_contact.c',
      'tests/unittests/test_preferences.c',
      'tests/unittests/test_server_events.c',
      'tests/unittests/test_muc.c',
      'tests/unittests/test_cmd_presence.c',
      'tests/unittests/test_cmd_alias.c',
      'tests/unittests/test_cmd_connect.c',
      'tests/unittests/test_cmd_rooms.c',
      'tests/unittests/test_cmd_account.c',
      'tests/unittests/test_cmd_sub.c',
      'tests/unittests/test_cmd_bookmark.c',
      'tests/unittests/test_cmd_otr.c',
      'tests/unittests/test_cmd_pgp.c',
      'tests/unittests/test_cmd_join.c',
      'tests/unittests/test_cmd_roster.c',
      'tests/unittests/test_cmd_disconnect.c',
      'tests/unittests/test_callbacks.c',
      'tests/unittests/test_plugins_disco.c',
      'tests/unittests/unittests.c',
    )
    
    if build_python_api
      unittest_sources += files(
        'src/plugins/python_plugins.c',
        'src/plugins/python_api.c',
      )
    endif
    
    if build_c_api
      unittest_sources += files(
        'src/plugins/c_plugins.c',
        'src/plugins/c_api.c',
      )
    endif
    
    if build_pgp
      unittest_sources += files(
        'tests/unittests/pgp/stub_gpg.c',
        'tests/unittests/pgp/stub_ox.c',
      )
    endif
    
    if build_otr
      unittest_sources += files('tests/unittests/otr/stub_otr.c')
    endif
    
    if build_omemo
      unittest_sources += files('tests/unittests/omemo/stub_omemo.c')
    endif
    
    unittests = executable(
      'unittests',
      unittest_sources,
      dependencies: profanity_deps + [cmocka_dep],
      include_directories: [inc, include_directories('tests')],
      build_by_default: false,
    )
    
    test('unit tests', unittests)
  else
    warning('cmocka not found, tests will not be built')
  endif
endif

summary({
  'Platform': platform,
  'Package status': get_option('buildtype'),
  'Install themes': get_option('install_themes'),
  'Themes path': themes_path,
  'Icons path': icons_path,
  'Global Python plugins path': global_python_plugins_path,
  'Global C plugins path': global_c_plugins_path,
}, section: 'Directories')

summary({
  'Notifications': libnotify_dep.found() or have_osxnotify,
  'Python plugins': build_python_api,
  'C plugins': build_c_api,
  'OTR': build_otr,
  'PGP': build_pgp,
  'OMEMO': build_omemo,
  'XScreenSaver': xscrnsaver_dep.length() > 0,
  'GTK': gtk_dep.found(),
  'GDK Pixbuf': gdk_pixbuf_dep.found(),
  'QR Code': qrencode_dep.found(),
}, section: 'Features')
