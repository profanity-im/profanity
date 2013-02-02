bin_PROGRAMS = profanity
profanity_SOURCES = src/command.c src/contact.c src/command_history.c \
	src/preferences.c src/autocomplete.c src/command.h src/contact.h \
	src/log.c src/preferences.h src/autocomplete.h src/common.c \
	src/contact_list.c src/log.h src/profanity.c src/history.c src/common.h \
	src/contact_list.h src/main.c src/profanity.h src/history.h src/tinyurl.c \
	src/tinyurl.h src/chat_session.c src/chat_session.h src/muc.c src/muc.h \
	src/command_parser.c src/theme.c src/theme.h src/files.c src/files.h \
	src/accounts.c src/accounts.h src/jid.h src/jid.c \
	src/xmpp/xmpp.h src/xmpp/capabilities.c src/xmpp/connection.c \
	src/xmpp/iq.c src/xmpp/message.c src/xmpp/presence.c src/xmpp/stanza.c \
	src/xmpp/stanza.h src/xmpp/message.h src/xmpp/iq.h src/xmpp/presence.h \
	src/ui/ui.h src/ui/window.c src/ui/window.h src/ui/windows.c \
	src/ui/titlebar.c src/ui/statusbar.c src/ui/inputwin.c

TESTS = tests/testsuite
check_PROGRAMS = tests/testsuite
tests_testsuite_SOURCES = tests/test_contact_list.c src/contact_list.c src/contact.c \
	tests/test_common.c tests/test_history.c src/history.c src/common.c \
	tests/test_autocomplete.c src/autocomplete.c tests/testsuite.c \
	tests/test_parser.c src/command_parser.c tests/test_jid.c src/jid.c
tests_testsuite_LDADD = -lheadunit -lstdc++

man_MANS = docs/profanity.1
