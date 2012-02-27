CC = gcc
WARNS = -Werror -Wall -Wextra -Wno-unused-parameter -Wno-unused-but-set-variable
LIBS = -lxml2 -lexpat -lssl -lresolv -lncurses -L ~/lib -lstrophe
CFLAGS = -I ~/include -O3 $(WARNS) $(LIBS)
OBJS = log.o windows.o title_bar.o status_bar.o input_win.o jabber.o \
       profanity.o util.o command.o history.o main.o

profanity: $(OBJS)
	$(CC) -o profanity $(OBJS) $(LIBS)

log.o: log.h
windows.o: windows.h util.h
title_bar.o: windows.h
status_bar.o: windows.h util.h
input_win.o: windows.h
jabber.o: jabber.h log.h windows.h
profanity.o: log.h windows.h jabber.h command.h history.h
util.o: util.h
command.o: command.h util.h history.h
history.o: history.h
main.o: log.h windows.h profanity.h

.PHONY: clean
clean:
	rm -f profanity
	rm -f profanity.log
	rm -f *.o
