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
	src/xmpp/form.c src/xmpp/form.h \
	src/server_events.c src/server_events.h \
	src/ui/ui.h src/ui/window.c src/ui/window.h src/ui/core.c \
	src/ui/titlebar.c src/ui/statusbar.c src/ui/inputwin.c \
	src/ui/titlebar.h src/ui/statusbar.h src/ui/inputwin.h \
	src/ui/console.c src/ui/notifier.c \
	src/ui/windows.c src/ui/windows.h \
	src/ui/rosterwin.c src/ui/occupantswin.c \
	src/ui/buffer.c src/ui/buffer.h \
	src/command/command.h src/command/command.c src/command/history.c \
	src/command/commands.h src/command/commands.c \
	src/command/history.h src/tools/parser.c \
	src/tools/parser.h \
	src/tools/p_sha1.h src/tools/p_sha1.c \
	src/tools/autocomplete.c src/tools/autocomplete.h \
	src/tools/history.c src/tools/history.h \
	src/tools/tinyurl.c src/tools/tinyurl.h \
	src/config/accounts.c src/config/accounts.h \
	src/config/account.c src/config/account.h \
	src/config/preferences.c src/config/preferences.h \
	src/config/theme.c src/config/theme.h

tests_sources = \
	src/contact.c src/contact.h src/common.c \
	src/log.h src/profanity.c src/common.h \
	src/profanity.h src/chat_session.c \
	src/chat_session.h src/muc.c src/muc.h src/jid.h src/jid.c \
	src/resource.c src/resource.h \
	src/roster_list.c src/roster_list.h \
	src/xmpp/xmpp.h src/xmpp/form.c \
	src/ui/ui.h \
	src/command/command.h src/command/command.c src/command/history.c \
	src/command/commands.h src/command/commands.c \
	src/command/history.h src/tools/parser.c \
	src/tools/parser.h \
	src/tools/p_sha1.h src/tools/p_sha1.c \
	src/tools/autocomplete.c src/tools/autocomplete.h \
	src/tools/history.c src/tools/history.h \
	src/tools/tinyurl.c src/tools/tinyurl.h \
	src/config/accounts.h \
	src/config/account.c src/config/account.h \
	src/config/preferences.c src/config/preferences.h \
	src/config/theme.c src/config/theme.h \
	src/ui/windows.c src/ui/windows.h \
	src/ui/window.c src/ui/window.h \
	src/ui/buffer.c \
	src/ui/titlebar.c src/ui/statusbar.c src/ui/inputwin.c \
	src/ui/titlebar.h src/ui/statusbar.h src/ui/inputwin.h \
	src/server_events.c src/server_events.h \
	tests/xmpp/stub_xmpp.c \
	tests/otr/stub_otr.c \
	tests/ui/stub_ui.c \
	tests/log/stub_log.c \
	tests/config/stub_accounts.c \
	tests/helpers.c tests/helpers.h \
	tests/test_cmd_account.c tests/test_cmd_account.h \
	tests/test_cmd_alias.c tests/test_cmd_alias.h \
	tests/test_cmd_bookmark.c tests/test_cmd_bookmark.h \
	tests/test_cmd_connect.c tests/test_cmd_connect.h \
	tests/test_cmd_join.c tests/test_cmd_join.h \
	tests/test_cmd_otr.c tests/test_cmd_otr.h \
	tests/test_cmd_rooms.c tests/test_cmd_rooms.h \
	tests/test_cmd_roster.c tests/test_cmd_roster.h \
	tests/test_cmd_statuses.c tests/test_cmd_statuses.h \
	tests/test_cmd_sub.c tests/test_cmd_sub.h \
	tests/test_cmd_win.c tests/test_cmd_win.h \
	tests/test_common.c tests/test_common.h \
	tests/test_contact.c tests/test_contact.h \
	tests/test_form.c tests/test_form.h \
	tests/test_history.c tests/test_history.h \
	tests/test_jid.c tests/test_jid.h \
	tests/test_muc.c tests/test_muc.h \
	tests/test_parser.c tests/test_parser.h \
	tests/test_preferences.c tests/test_preferences.h \
	tests/test_roster_list.c tests/test_roster_list.h \
	tests/test_server_events.c tests/test_server_events.h \
	tests/test_autocomplete.c tests/test_autocomplete.h \
	tests/testsuite.c

main_source = src/main.c

git_include = src/gitversion.h

otr3_sources = \
	src/otr/otrlib.h src/otr/otrlibv3.c src/otr/otr.h src/otr/otr.c

otr4_sources = \
	src/otr/otrlib.h src/otr/otrlibv4.c src/otr/otr.h src/otr/otr.c

themes_sources = themes/*

script_sources = bootstrap.sh configure-debug install-all.sh

man_sources = docs/profanity.1

if BUILD_OTR
if BUILD_OTR3
core_sources += $(otr3_sources)
endif
if BUILD_OTR4
core_sources += $(otr4_sources)
endif
endif

bin_PROGRAMS = profanity
profanity_SOURCES = $(core_sources) $(main_source)
if THEMES_INSTALL
profanity_themesdir = @THEMES_PATH@
profanity_themes_DATA = $(themes_sources)
endif
if INCLUDE_GIT_VERSION
BUILT_SOURCES = $(git_include)
endif

TESTS = tests/testsuite
check_PROGRAMS = tests/testsuite
tests_testsuite_SOURCES = $(tests_sources)
tests_testsuite_LDADD = -lcmocka

man_MANS = $(man_sources)

EXTRA_DIST = $(man_sources) $(themes_sources) $(script_sources) profrc.example LICENSE.txt

if INCLUDE_GIT_VERSION
EXTRA_DIST += .git/HEAD .git/index

$(git_include).in: .git/HEAD .git/index
	rm -f $@
	echo "#ifndef PROF_GIT_BRANCH" >> $@
	echo "#define PROF_GIT_BRANCH \"$(shell git rev-parse --symbolic-full-name --abbrev-ref HEAD)\"" >> $@
	echo "#endif" >> $@
	echo "#ifndef PROF_GIT_REVISION" >> $@
	echo "#define PROF_GIT_REVISION \"$(shell git log --pretty=format:'%h' -n 1)\"" >> $@
	echo "#endif" >> $@

#
# Create $(git_include) atomically to catch possible race. The race can occur
# when $(git_include) is generated in parallel with building of src/profanity.c.
# So this hack allows to find and fix the problem earlier.
#
$(git_include): $(git_include).in
	cp $< $@

clean-local:
	rm -f $(git_include) $(git_include).in
endif
