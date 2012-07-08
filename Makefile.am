bin_PROGRAMS = profanity
profanity_SOURCES = src/command.c src/contact.c src/history.c src/jabber.h \
	src/preferences.c src/prof_autocomplete.c src/status_bar.c src/util.h \
	src/command.h src/contact.h src/history.h src/log.c src/preferences.h \
	src/prof_autocomplete.h src/title_bar.c src/windows.c src/common.c \
	src/contact_list.c src/input_win.c src/log.h src/profanity.c \
	src/prof_history.c src/ui.h src/common.h src/ contact_list.h src/jabber.c \
	src/main.c src/profanity.h src/prof_history.h src/util.c
profanity_CFLAGS = -O3 -Werror -Wall -Wextra -Wno-unused-parameter \
	-Wno-unused-but-set-variable -Wno-unused-result \
	-lstrophe -lxml2 -lexpat $(DEPS_CFLAGS) $(DEPS_LIBS) -lresolv \
	$(NOTIFY_CFLAGS) $(NOTIFY_LIBS)

TESTS = tests/testsuite
check_PROGRAMS = tests/testsuite
tests_testsuite_SOURCES = tests/test_contact_list.c src/contact_list.c src/contact.c \
	tests/test_util.c tests/test_prof_history.c src/prof_history.c src/util.c \
	tests/test_prof_autocomplete.c src/prof_autocomplete.c src/common.c tests/testsuite.c
tests_testsuite_CFLAGS = -I ~/include
tests_testsuite_LDADD = -L ~/lib -lheadunit -lstdc++

