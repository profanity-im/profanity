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
	src/config/account.c src/config/account.h \
	src/config/preferences.c src/config/preferences.h \
	src/config/theme.c src/config/theme.h

test_sources = \
	src/contact.c src/contact.h src/common.c \
	src/log.h src/profanity.c src/common.h \
	src/profanity.h src/chat_session.c \
	src/chat_session.h src/muc.c src/muc.h src/jid.h src/jid.c \
	src/resource.c src/resource.h \
	src/roster_list.c src/roster_list.h \
	src/xmpp/xmpp.h \
	src/ui/ui.h \
	src/command/command.h src/command/command.c src/command/history.c \
	src/command/commands.h src/command/commands.c \
	src/command/history.h src/tools/parser.c \
	src/tools/parser.h \
	src/tools/autocomplete.c src/tools/autocomplete.h \
	src/tools/history.c src/tools/history.h \
	src/tools/tinyurl.c src/tools/tinyurl.h \
	src/config/accounts.h \
	src/config/account.c src/config/account.h \
	src/config/preferences.c src/config/preferences.h \
	src/config/theme.c src/config/theme.h \
	src/ui/windows.c src/ui/windows.h \
	src/ui/window.c src/ui/window.h \
	src/server_events.c src/server_events.h \
	tests/xmpp/mock_xmpp.h tests/xmpp/mock_xmpp.c \
	tests/otr/mock_otr.h tests/otr/mock_otr.c \
	tests/ui/mock_ui.h tests/ui/mock_ui.c \
	tests/config/mock_accounts.h tests/config/mock_accounts.c \
	tests/helpers.c tests/helpers.h \
	tests/log/mock_log.c \
	tests/test_autocomplete.c \
	tests/test_common.c \
	tests/test_contact.c \
	tests/test_cmd_connect.c \
	tests/test_cmd_account.c \
	tests/test_cmd_rooms.c \
	tests/test_cmd_sub.c \
	tests/test_cmd_alias.c \
	tests/test_cmd_statuses.c \
	tests/test_cmd_bookmark.c \
	tests/test_cmd_otr.c \
	tests/test_history.c \
	tests/test_jid.c \
	tests/test_parser.c \
	tests/test_roster_list.c \
	tests/test_preferences.c \
	tests/test_server_events.c \
	tests/test_muc.c \
	tests/testsuite.c

main_source = src/main.c

git_sources = \
	src/gitversion.c

otr3_sources = \
	src/otr/otrlib.h src/otr/otrlibv3.c src/otr/otr.h src/otr/otr.c

otr4_sources = \
	src/otr/otrlib.h src/otr/otrlibv4.c src/otr/otr.h src/otr/otr.c

if INCLUDE_GIT_VERSION
with_git_sources = $(git_sources) $(core_sources)
tests_with_git_sources = $(git_sources) $(test_sources)
else
with_git_sources = $(core_sources)
tests_with_git_sources = $(test_sources)
endif

if BUILD_OTR
if BUILD_OTR3
with_otr_sources = $(with_git_sources) $(otr3_sources)
tests_with_otr_sources = $(tests_with_git_sources) $(otr3_sources)
endif
if BUILD_OTR4
with_otr_sources = $(with_git_sources) $(otr4_sources)
tests_with_otr_sources = $(tests_with_git_sources) $(otr4_sources)
endif
else
with_otr_sources = $(with_git_sources)
tests_with_otr_sources = $(tests_with_git_sources)
endif

bin_PROGRAMS = profanity
profanity_SOURCES = $(with_otr_sources) $(main_source)

TESTS = tests/testsuite
check_PROGRAMS = tests/testsuite
tests_testsuite_SOURCES = $(tests_with_otr_sources)
tests_testsuite_LDADD = -lcmocka

man_MANS = docs/profanity.1
