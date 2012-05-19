CC = gcc
WARNS = -Werror -Wall -Wextra -Wno-unused-parameter -Wno-unused-but-set-variable \
	-Wno-unused-result
LIBS = -lxml2 -lexpat -lssl -lresolv -lncurses -L ~/lib -lstrophe `pkg-config --libs glib-2.0`
TESTLIB = -L ~/lib -l headunit
CPPLIB = -lstdc++
CFLAGS = -I ~/include -O3 $(WARNS) $(LIBS) `pkg-config --cflags glib-2.0`
OBJS = log.o windows.o title_bar.o status_bar.o input_win.o jabber.o \
       profanity.o util.o command.o history.o contact_list.o prof_history.o \
	   contact.o preferences.o prof_autocomplete.o common.o main.o
TESTOBJS = test_contact_list.o contact_list.o contact.o \
	       test_util.o test_prof_history.o prof_history.o util.o \
		   test_prof_autocomplete.o prof_autocomplete.o

profanity: $(OBJS)
	$(CC) -o profanity $(OBJS) $(LIBS)

common.o: common.h
log.o: log.h
windows.o: windows.h util.h contact_list.h preferences.h
title_bar.o: windows.h
status_bar.o: windows.h util.h
input_win.o: windows.h preferences.h
jabber.o: jabber.h log.h windows.h contact_list.h
profanity.o: log.h windows.h jabber.h command.h preferences.h \
			 contact_list.h
util.o: util.h
command.o: command.h util.h history.h contact_list.h
history.o: history.h prof_history.h
contact_list.o: contact_list.h contact.h prof_autocomplete.h
prof_history.o: prof_history.h
contact.o: contact.h
preferences.o: preferences.h
prof_autocomplete.o: prof_autocomplete.h
main.o: profanity.h

test_contact_list.o: contact_list.h contact.h
test_util.o: util.h
test_prof_history.o: prof_history.h
test_prof_autocomplete.o: contact.h prof_autocomplete.h

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
