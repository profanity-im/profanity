bin_PROGRAMS = profanity
profanity_SOURCES = src/command.c src/contact.c src/command_history.c src/xmpp.h \
	src/preferences.c src/autocomplete.c src/ui_statusbar.c \
	src/command.h src/contact.h src/log.c src/preferences.h \
	src/autocomplete.h src/ui_titlebar.c src/ui_windows.c src/common.c \
	src/contact_list.c src/ui_inputwin.c src/log.h src/profanity.c \
	src/history.c src/ui.h src/common.h src/ contact_list.h src/xmpp_conn.c \
	src/main.c src/profanity.h src/history.h  src/xmpp_presence.c \
	src/tinyurl.c src/tinyurl.h src/chat_session.c \
	src/chat_session.h src/muc.c \
	src/muc.h src/xmpp_stanza.c src/command_parser.c \
	src/theme.c src/theme.h src/window.c src/window.h \
	src/files.c src/files.h src/accounts.c src/accounts.h \
	src/jid.h src/jid.c src/xmpp_caps.c src/xmpp_iq.c

TESTS = tests/testsuite
check_PROGRAMS = tests/testsuite
tests_testsuite_SOURCES = tests/test_contact_list.c src/contact_list.c src/contact.c \
	tests/test_common.c tests/test_history.c src/history.c src/common.c \
	tests/test_autocomplete.c src/autocomplete.c tests/testsuite.c \
	tests/test_parser.c src/command_parser.c tests/test_jid.c src/jid.c
tests_testsuite_LDADD = -lheadunit -lstdc++

man_MANS = docs/profanity.1
