if INCLUDE_GIT_VERSION
src/gitversion.c: .git/HEAD .git/index
	rm -f src/gitversion.c src/gitversion.o
	echo "#ifndef PROF_GIT_BRANCH" >> $@
	echo "#define PROF_GIT_BRANCH \"$(shell git rev-parse --symbolic-full-name --abbrev-ref HEAD)\"" >> $@
	echo "#endif" >> $@
	echo "#ifndef PROF_GIT_REVISION" >> $@
	echo "#define PROF_GIT_REVISION \"$(shell git log --pretty=format:'%h' -n 1)\"" >> $@
	echo "#endif" >> $@

clean-local:
	rm -f src/gitversion.c src/gitversion.o
endif

core_sources = \
	src/contact.c src/contact.h src/log.c src/common.c \
	src/log.h src/profanity.c src/common.h \
	src/profanity.h src/chat_session.c \
	src/chat_session.h src/muc.c src/muc.h src/jid.h src/jid.c \
	src/resource.c src/resource.h \
	src/roster_list.c src/roster_list.h \
	src/xmpp/xmpp.h src/xmpp/capabilities.c src/xmpp/connection.c \
	src/xmpp/iq.c src/xmpp/message.c src/xmpp/presence.c src/xmpp/stanza.c \
	src/xmpp/stanza.h src/xmpp/message.h src/xmpp/iq.h src/xmpp/presence.h \
	src/xmpp/capabilities.h src/xmpp/connection.h \
	src/xmpp/roster.c src/xmpp/roster.h \
	src/xmpp/bookmark.c src/xmpp/bookmark.h \
    src/server_events.c src/server_events.h \
	src/ui/ui.h src/ui/window.c src/ui/window.h src/ui/core.c \
	src/ui/titlebar.c src/ui/statusbar.c src/ui/inputwin.c \
	src/ui/console.c src/ui/notifier.c \
    src/ui/windows.c src/ui/windows.h \
	src/command/command.h src/command/command.c src/command/history.c \
	src/command/commands.h src/command/commands.c \
	src/command/history.h src/tools/parser.c \
	src/tools/parser.h \
	src/tools/autocomplete.c src/tools/autocomplete.h \
	src/tools/history.c src/tools/history.h \
	src/tools/tinyurl.c src/tools/tinyurl.h \
	src/config/accounts.c src/config/accounts.h \
	src/config/preferences.c src/config/preferences.h \
	src/config/theme.c src/config/theme.h \
    src/plugins/plugins.h src/plugins/plugins.c \
    src/plugins/api.h src/plugins/api.c \
    src/plugins/callbacks.h src/plugins/callbacks.c

test_sources = \
	src/contact.c src/contact.h src/common.c \
	src/log.h src/profanity.c src/common.h \
	src/profanity.h src/chat_session.c \
	src/chat_session.h src/muc.c src/muc.h src/jid.h src/jid.c \
	src/resource.c src/resource.h \
	src/roster_list.c src/roster_list.h \
	src/xmpp/xmpp.h \
	src/ui/ui.h src/ui/window.c src/ui/window.h \
    src/ui/windows.c src/ui/windows.h \
    src/ui/muc_window.c src/ui/muc_window.h \
	src/command/command.h src/command/command.c src/command/history.c \
	src/command/commands.h src/command/commands.c \
	src/command/history.h src/tools/parser.c \
	src/tools/parser.h \
	src/tools/autocomplete.c src/tools/autocomplete.h \
	src/tools/history.c src/tools/history.h \
	src/tools/tinyurl.c src/tools/tinyurl.h \
	src/config/accounts.h \
	src/config/preferences.c src/config/preferences.h \
	src/config/theme.c src/config/theme.h \
    src/plugins/plugins.h src/plugins/plugins.c \
    src/plugins/api.h src/plugins/api.c \
    src/plugins/callbacks.h src/plugins/callbacks.c \
    tests/xmpp/mock_xmpp.h tests/xmpp/mock_xmpp.c \
    tests/ui/mock_ui.h tests/ui/mock_ui.c \
    tests/config/mock_accounts.h tests/config/mock_accounts.c \
    tests/log/mock_log.c \
    tests/test_autocomplete.c \
    tests/test_common.c \
    tests/test_contact.c \
	tests/test_cmd_connect.c \
	tests/test_cmd_account.c \
	tests/test_cmd_rooms.c \
	tests/test_cmd_sub.c \
	tests/test_history.c \
	tests/test_jid.c \
	tests/test_parser.c \
	tests/test_roster_list.c \
    tests/testsuite.c

main_source = src/main.c

python_sources = \
    src/plugins/python_plugins.h src/plugins/python_plugins.c \
    src/plugins/python_api.h src/plugins/python_api.c

ruby_sources = \
    src/plugins/ruby_plugins.h src/plugins/ruby_plugins.c \
    src/plugins/ruby_api.h src/plugins/ruby_api.c

lua_sources = \
    src/plugins/lua_plugins.h src/plugins/lua_plugins.c \
    src/plugins/lua_api.h src/plugins/lua_api.c

c_sources = \
    src/plugins/c_plugins.h src/plugins/c_plugins.c \
    src/plugins/c_api.h src/plugins/c_api.c

git_sources = \
    src/gitversion.c

if BUILD_PYTHON_API
with_python_sources = $(core_sources) $(python_sources)
tests_with_python_sources = $(test_sources) $(python_sources)
else
with_python_sources = $(core_sources)
tests_with_python_sources = $(test_sources)
endif

if BUILD_RUBY_API
with_ruby_sources = $(with_python_sources) $(ruby_sources)
tests_with_ruby_sources = $(tests_with_python_sources) $(ruby_sources)
else
with_ruby_sources = $(with_python_sources)
tests_with_ruby_sources = $(tests_with_python_sources)
endif

if BUILD_LUA_API
with_lua_sources = $(with_ruby_sources) $(lua_sources)
tests_with_lua_sources = $(tests_with_ruby_sources) $(lua_sources)
else
with_lua_sources = $(with_ruby_sources)
test_with_lua_sources = $(tests_with_ruby_sources)
endif

if BUILD_C_API
with_c_sources = $(with_lua_sources) $(c_sources)
tests_with_c_sources = $(tests_with_lua_sources) $(c_sources)
else
with_c_sources = $(with_lua_sources)
test_with_c_sources = $(tests_with_lua_sources)
endif

if INCLUDE_GIT_VERSION
with_git_sources = $(git_sources) $(with_c_sources)
tests_with_git_sources = $(git_sources) $(tests_with_c_sources)
else
with_git_sources = $(with_c_sources)
tests_with_git_sources = $(tests_with_c_sources)
endif

bin_PROGRAMS = profanity
profanity_SOURCES = $(with_git_sources) $(main_source)

if BUILD_C_API
lib_LTLIBRARIES = libprofanity.la
libprofanity_la_LDFLAGS=-module -avoid-version -shared
libprofanity_la_SOURCES = src/plugins/profapi.c

library_includedir=$(includedir)
library_include_HEADERS = src/plugins/profapi.h
endif

TESTS = tests/testsuite
check_PROGRAMS = tests/testsuite
tests_testsuite_SOURCES = $(tests_with_git_sources)
tests_testsuite_LDADD = -lcmocka

man_MANS = docs/profanity.1
