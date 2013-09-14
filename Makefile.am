core_sources = \
	src/contact.c src/contact.h src/log.c src/common.c \
	src/log.h src/profanity.c src/common.h \
	src/profanity.h src/chat_session.c \
	src/chat_session.h src/muc.c src/muc.h src/jid.h src/jid.c \
	src/resource.c src/resource.h \
	src/xmpp/xmpp.h src/xmpp/capabilities.c src/xmpp/connection.c \
	src/xmpp/iq.c src/xmpp/message.c src/xmpp/presence.c src/xmpp/stanza.c \
	src/xmpp/stanza.h src/xmpp/message.h src/xmpp/iq.h src/xmpp/presence.h \
	src/xmpp/capabilities.h src/xmpp/connection.h \
	src/xmpp/roster.c src/xmpp/roster.h \
	src/xmpp/bookmark.c src/xmpp/bookmark.h \
	src/ui/ui.h src/ui/window.c src/ui/window.h src/ui/core.c \
	src/ui/titlebar.c src/ui/statusbar.c src/ui/inputwin.c \
	src/ui/console.c src/ui/notifier.c src/ui/notifier.h \
    src/ui/windows.c src/ui/windows.h \
	src/command/command.h src/command/command.c src/command/history.c \
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
    src/plugins/callbacks.h src/plugins/callbacks.c \
    src/plugins/c_plugins.h src/plugins/c_plugins.c \
    src/plugins/ruby_plugins.h src/plugins/ruby_plugins.c \
    src/plugins/ruby_api.h src/plugins/ruby_api.c \
    src/plugins/c_api.h src/plugins/c_api.c

main_source = src/main.c

python_sources = \
    src/plugins/python_plugins.h src/plugins/python_plugins.c \
    src/plugins/python_api.h src/plugins/python_api.c

test_sources = \
	tests/test_roster.c tests/test_common.c tests/test_history.c \
	tests/test_autocomplete.c tests/testsuite.c tests/test_parser.c \
	tests/test_jid.c

if BUILD_PYTHON_API
all_sources = $(core_sources) $(python_sources)
else
all_sources = $(core_sources)
endif

bin_PROGRAMS = profanity
profanity_SOURCES = $(main_source) $(all_sources)

lib_LTLIBRARIES = libprofanity.la
libprofanity_la_LDFLAGS=-module -avoid-version -shared
libprofanity_la_SOURCES = src/plugins/profapi.c

library_includedir=$(includedir)
library_include_HEADERS = src/plugins/profapi.h

TESTS = tests/testsuite
check_PROGRAMS = tests/testsuite
tests_testsuite_SOURCES = $(all_sources) $(test_sources)
tests_testsuite_LDADD = -lheadunit -lstdc++

man_MANS = docs/profanity.1
