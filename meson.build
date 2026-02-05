project('profanity', 'c',
  version: '0.15.1',
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

# possible options from meson:
# debug, debugoptimized, release, plain
if get_option('buildtype') == 'release'
  conf_data.set_quoted('PACKAGE_STATUS', 'release')
else
  conf_data.set_quoted('PACKAGE_STATUS', 'development')
endif

if platform == 'cygwin'
  conf_data.set('PLATFORM_CYGWIN', 1)
endif

if platform == 'osx'
  conf_data.set('PLATFORM_OSX', 1)
endif

# Get git version if in development
if get_option('buildtype') == 'debug' or get_option('buildtype') == 'debugoptimized'
  git = find_program('git', required: false)
  if git.found()
    conf_data.set('HAVE_GIT_VERSION', 1)
  endif
endif

# Compiler setup
cc = meson.get_compiler('c')

# Common compiler flags
add_project_arguments('-Wno-deprecated-declarations', language: 'c')
add_project_arguments('-Wno-unused-parameter', language: 'c')

if get_option('buildtype') == 'debug' or get_option('buildtype') == 'debugoptimized'
  add_project_arguments('-ggdb3', language: 'c')
  add_project_arguments('-Wunused', language: 'c')
  #Disabled for now due so we can build
  #add_project_arguments('-Werror', language: 'c')
endif

if platform == 'osx'
  add_project_arguments('-Qunused-arguments', language: 'c')
  
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
math_dep = cc.find_library('m', required: true)
libstrophe_dep = dependency('libstrophe', version: '>= 0.12.3')

# Check if libstrophe works
if not cc.links('''
  #include <strophe.h>
  int main() {
    xmpp_initialize();
    return 0;
  }
  ''', dependencies: libstrophe_dep)
  error('libstrophe is broken')
endif

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

# Curses/ncurses library
ncursesw_dep = dependency('ncursesw', required: false)
ncurses_dep = dependency('ncurses', required: false)

if ncursesw_dep.found()
  curses_dep = ncursesw_dep
  curses_name = 'ncursesw'
elif ncurses_dep.found()
  curses_dep = ncurses_dep
  curses_name = 'ncurses'
else
  curses_dep = cc.find_library('ncursesw', required: false)
  if curses_dep.found()
    curses_name = 'ncursesw'
  else
    curses_dep = cc.find_library('ncurses', required: false)
    if curses_dep.found()
      curses_name = 'ncurses'
    else
      curses_dep = cc.find_library('curses', required: true)
      curses_name = 'curses'
    endif
  endif
endif

# Check for wide character support
if not cc.links('''
  void wget_wch(void);
  int main() {
    wget_wch();
    return 0;
  }
  ''', dependencies: curses_dep)
  error('ncurses does not support wide characters')
endif

# Check for ncurses headers
if cc.has_header('ncursesw/ncurses.h')
  conf_data.set('HAVE_NCURSESW_NCURSES_H', 1)
endif
if cc.has_header('ncurses.h')
  conf_data.set('HAVE_NCURSES_H', 1)
endif
if cc.has_header('curses.h')
  conf_data.set('HAVE_CURSES_H', 1)
endif

# Readline:
# we need to declare the variable before
# using it in the conditional branch.
# Later the required is set correctly.
readline_dep = dependency('', required: false)

if platform == 'osx'
  brew = find_program('brew', required: false, dirs: ['/opt/homebrew/bin', '/usr/local/bin'])
  
  if brew.found()
    readline_prefix_cmd = run_command(brew, '--prefix', 'readline', check: false)
    if readline_prefix_cmd.returncode() == 0
      readline_prefix = readline_prefix_cmd.stdout().strip()
    else
      readline_prefix = '/usr/local'
    endif
  else
    if run_command('test', '-f', '/opt/local/lib/libreadline.dylib', check: false).returncode() == 0
      readline_prefix = '/opt/local'
    else
      readline_prefix = '/usr/local'
    endif
  endif
  
  readline_dep = cc.find_library('readline', 
    dirs: [readline_prefix / 'lib'],
    required: true)
  add_project_arguments('-I' + readline_prefix / 'include', language: 'c')
elif platform == 'openbsd'
  if run_command('test', '-d', '/usr/local/include/ereadline', check: false).returncode() == 0
    readline_dep = cc.find_library('ereadline', 
      dirs: ['/usr/local/lib'],
      required: true)
    add_project_arguments('-I/usr/local/include/ereadline', language: 'c')
  else
    readline_dep = cc.find_library('readline', required: true)
  endif
else
  readline_dep = cc.find_library('readline', required: true)
endif

# Optional dependencies:
# Notifications
libnotify_dep = dependency('', required: false)
have_osxnotify = false

if get_option('notifications').enabled() or get_option('notifications').auto()
  if platform == 'osx'
    terminal_notifier = find_program('terminal-notifier', required: get_option('notifications'))
    if terminal_notifier.found()
      have_osxnotify = true
      conf_data.set('HAVE_OSXNOTIFY', 1)
    endif
  elif platform == 'nix' or platform == 'freebsd'
    libnotify_dep = dependency('libnotify', required: get_option('notifications'))
    if libnotify_dep.found()
      conf_data.set('HAVE_LIBNOTIFY', 1)
    endif
  endif
endif

# GTK (for icons and clipboard)
gtk_dep = dependency('', required: false)
gtk_version = 'none'

if get_option('icons-and-clipboard').enabled() or get_option('icons-and-clipboard').auto()
  gtk_dep = dependency('gtk+-3.0', version: '>= 3.24.0', required: false)
  if gtk_dep.found()
    conf_data.set('HAVE_GTK', 1)
    gtk_version = gtk_dep.version()
  else
    gtk_dep = dependency('gtk+-2.0', version: '>= 2.24.10', 
                        required: get_option('icons-and-clipboard'))
    if gtk_dep.found()
      conf_data.set('HAVE_GTK', 1)
      gtk_version = gtk_dep.version()
    endif
  endif
endif

# XScreenSaver
# We use this variable because .found() will later not work on a list
xscrnsaver_found = false
xscrnsaver_dep = []

if get_option('xscreensaver').enabled() or get_option('xscreensaver').auto()
  xss_dep = dependency('xscrnsaver', required: false)
  x11_dep = dependency('x11', required: false)
  
  if xss_dep.found() and x11_dep.found()
    xscrnsaver_dep = [xss_dep, x11_dep]
    xscrnsaver_found = true # Set our tracker to true
    conf_data.set('HAVE_LIBXSS', 1)
  elif get_option('xscreensaver').enabled()
    error('xscreensaver support requested but xscrnsaver or x11 not found')
  endif
endif

# Python plugins
python_dep = dependency('', required: false)
build_python_api = false

if not get_option('plugins').disabled() and not get_option('python-plugins').disabled()
  if platform == 'osx'
    # Handle Python framework on macOS
    python_framework = get_option('python_framework')
    if python_framework == ''
      python_framework = '/Library/Frameworks/Python.framework'
    endif
  endif
  
  python_dep = dependency('python-embed', required: false)
  if not python_dep.found()
    python_dep = dependency('python3-embed', required: false)
    if python_dep.found()
      conf_data.set('PY_IS_PYTHON3', 1)
    endif
  endif
  
  if python_dep.found()
    build_python_api = true
    conf_data.set('HAVE_PYTHON', 1)
  elif get_option('python-plugins').enabled()
    error('Python plugins enabled but Python not found')
  endif
endif

# C plugins
dl_dep = dependency('', required: false)
build_c_api = false

if platform != 'cygwin'
  if not get_option('plugins').disabled() and not get_option('c-plugins').disabled()
    if platform == 'openbsd' or platform == 'freebsd' or platform == 'netbsd'
      build_c_api = true
      conf_data.set('HAVE_C', 1)
    else
      dl_dep = cc.find_library('dl', required: get_option('c-plugins'))
      if dl_dep.found()
        build_c_api = true
        conf_data.set('HAVE_C', 1)
      endif
    endif
  endif
endif

# PGP support
gpgme_dep = dependency('', required: false)
build_pgp = false

if not get_option('pgp').disabled()
  gpgme_dep = cc.find_library('gpgme', required: false)
  
  if gpgme_dep.found()
    gpgme_config = find_program('gpgme-config', required: false)
    if gpgme_config.found()
      gpgme_cflags = run_command(gpgme_config, '--cflags', check: true).stdout().strip()
      gpgme_libs = run_command(gpgme_config, '--libs', check: true).stdout().strip()
      add_project_arguments(gpgme_cflags.split(), language: 'c')
    endif
    
    build_pgp = true
    conf_data.set('HAVE_LIBGPGME', 1)
  elif get_option('pgp').enabled()
    error('PGP support enabled but libgpgme not found')
  endif
endif

# OTR support
libotr_dep = dependency('', required: false)
build_otr = false

if not get_option('otr').disabled()
  libotr_dep = dependency('libotr', version: '>= 4.0', required: get_option('otr'))
  if libotr_dep.found()
    build_otr = true
    conf_data.set('HAVE_LIBOTR', 1)
  endif
endif

# GDK Pixbuf (for avatar scaling)
gdk_pixbuf_dep = dependency('', required: false)

if not get_option('gdk-pixbuf').disabled()
  gdk_pixbuf_dep = dependency('gdk-pixbuf-2.0', version: '>= 2.4', 
                              required: get_option('gdk-pixbuf'))
  if gdk_pixbuf_dep.found()
    conf_data.set('HAVE_PIXBUF', 1)
  endif
endif

# OMEMO support
libsignal_dep = dependency('', required: false)
gcrypt_dep = dependency('', required: false)
build_omemo = false

if not get_option('omemo').disabled()
  libsignal_dep = dependency('libsignal-protocol-c', version: '>= 2.3.2', required: false)
  
  if libsignal_dep.found()
    gcrypt_dep = dependency('libgcrypt', version: '>= 1.7.0', required: false)
    
    if gcrypt_dep.found()
      build_omemo = true
      conf_data.set('HAVE_OMEMO', 1)
    elif get_option('omemo').enabled()
      error('OMEMO support enabled but libgcrypt >= 1.7.0 not found')
    endif
  elif get_option('omemo').enabled()
    error('OMEMO support enabled but libsignal-protocol-c >= 2.3.2 not found')
  endif
endif

# QR code support (for OMEMO)
qrencode_dep = dependency('', required: false)

if not get_option('omemo-qrcode').disabled()
  qrencode_dep = dependency('libqrencode', required: get_option('omemo-qrcode'))
  if qrencode_dep.found()
    conf_data.set('HAVE_QRENCODE', 1)
  endif
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
  install: false,
)

# Make config.h available by adding build directory to include path
config_h_inc = include_directories('.')

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

if libnotify_dep.found()
  profanity_deps += libnotify_dep
endif

if gtk_dep.found()
  profanity_deps += gtk_dep
endif

if xscrnsaver_found
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

if libsignal_dep.found() and gcrypt_dep.found()
  profanity_deps += [libsignal_dep, gcrypt_dep]
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

# Python plugin sources
python_sources = files(
  'src/plugins/python_plugins.c',
  'src/plugins/python_api.c',
)

# C plugin sources
c_sources = files(
  'src/plugins/c_plugins.c',
  'src/plugins/c_api.c',
)

# PGP sources
pgp_sources = files(
  'src/pgp/gpg.c',
  'src/pgp/ox.c',
)

# OTR sources
otr_sources = files(
  'src/otr/otrlibv4.c',
  'src/otr/otr.c',
)

# OMEMO sources
omemo_sources = files(
  'src/omemo/omemo.c',
  'src/omemo/crypto.c',
  'src/omemo/store.c',
  'src/xmpp/omemo.c',
  'src/tools/aesgcm_download.c',
)

# Build the final source list
profanity_sources = core_sources

# Add tray support only when GTK is available
# The calls to tray functions in profanity.c are already conditional on HAVE_GTK
if gtk_dep.found()
  profanity_sources += files('src/ui/tray.c')
endif

if build_python_api
  profanity_sources += python_sources
endif

if build_c_api
  profanity_sources += c_sources
endif

if build_pgp
  profanity_sources += pgp_sources
endif

if build_otr
  profanity_sources += otr_sources
endif

if build_omemo
  profanity_sources += omemo_sources
endif

# Main source file
main_source = files('src/main.c')

# Generate git version header if in development
if get_option('buildtype') == 'release' or get_option('buildtype') == 'debugoptimized'
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
    install: false,
  )
endif

# Build the executable
profanity_exe = executable(
  'profanity',
  profanity_sources + main_source + [config_h],
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
  
  # Install profapi.h header
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

# Install example config
install_data('profrc.example',
  install_dir: get_option('datadir') / 'doc' / meson.project_name(),
)

install_data('theme_template',
  install_dir: get_option('datadir') / 'doc' / meson.project_name(),
)

# Tests
if get_option('tests')
  cmocka_dep = dependency('cmocka', required: false)
  if cmocka_dep.found()
    # Unit test sources
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
      unittest_sources += python_sources
    endif
    
    if build_c_api
      unittest_sources += c_sources
    endif
    
    if build_pgp
      unittest_sources += files(
        'tests/unittests/pgp/stub_gpg.c',
        'tests/unittests/pgp/stub_ox.c',
      )
    endif
    
    if build_otr
      unittest_sources += files(
        'tests/unittests/otr/stub_otr.c',
      )
    endif
    
    if build_omemo
      unittest_sources += files(
        'tests/unittests/omemo/stub_omemo.c',
      )
    endif
    
    # Build unit tests
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
  'GTK version': gtk_version,
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
  'XScreenSaver': xscrnsaver_found,
  'GTK': gtk_dep.found(),
  'GDK Pixbuf': gdk_pixbuf_dep.found(),
  'QR Code': qrencode_dep.found(),
}, section: 'Features')
