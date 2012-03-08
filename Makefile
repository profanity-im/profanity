CC = gcc
WARNS = -Werror -Wall -Wextra -Wno-unused-parameter -Wno-unused-but-set-variable
LIBS = -lxml2 -lexpat -lssl -lresolv -lncurses -L ~/lib -lstrophe
TESTLIB = -L ~/lib -l headunit
CPPLIB = -lstdc++
CFLAGS = -I ~/include -O3 $(WARNS) $(LIBS)
OBJS = log.o windows.o title_bar.o status_bar.o input_win.o jabber.o \
       profanity.o util.o command.o history.o contact_list.o main.o
TESTOBJS = test_history.o history.o test_contact_list.o contact_list.o

profanity: $(OBJS)
	$(CC) -o profanity $(OBJS) $(LIBS)

log.o: log.h
windows.o: windows.h util.h
title_bar.o: windows.h
status_bar.o: windows.h util.h
input_win.o: windows.h
jabber.o: jabber.h log.h windows.h contact_list.h
profanity.o: log.h windows.h jabber.h command.h history.h
util.o: util.h
command.o: command.h util.h history.h contact_list.h
history.o: history.h
contact_list.o: contact_list.h
main.o: profanity.h

test_history.o: history.h
test_contact_list.o: contact_list.h

testsuite: testsuite.h $(TESTOBJS)
	$(CC) $(CFLAGS) $(CPPLIB) testsuite.c $(TESTOBJS) -o testsuite $(TESTLIB)

.PHONY: test
test: testsuite
	./testsuite

.PHONY: clean
clean:
	rm -f profanity
	rm -f profanity.log
	rm -f *.o
	rm -f testsuite
