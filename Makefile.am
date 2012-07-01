CFLAGS = -O3 -Werror -Wall -Wextra -Wno-unused-parameter -Wno-unused-but-set-variable \
	-Wno-unused-result -lncurses -lstrophe -lxml2 -lexpat -lssl -lresolv \
		`pkg-config --libs --cflags glib-2.0` `pkg-config --libs --cflags libnotify`


bin_PROGRAMS = profanity
profanity_SOURCES = src/command.c src/contact.c src/history.c src/jabber.h \
	src/preferences.c src/prof_autocomplete.c src/status_bar.c src/util.h \
	src/command.h src/contact.h src/history.h src/log.c src/preferences.h \
	src/prof_autocomplete.h src/title_bar.c src/windows.c src/common.c \
	src/contact_list.c src/input_win.c src/log.h src/profanity.c \
	src/prof_history.c src/ui.h src/common.h src/ contact_list.h src/jabber.c \
	src/main.c src/profanity.h src/prof_history.h src/util.c

