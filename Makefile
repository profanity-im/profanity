CC = gcc
CFLAGS = -O3 -Werror -Wall -Wextra -Wno-unused-parameter -Wno-unused-but-set-variable
LIBS = -lxml2 -lssl -lresolv -lncurses -lstrophe

profanity: clean
	$(CC) profanity.c $(LIBS) -o profanity 
	$(CC) curses_example.c -lncurses -o curses_example

.PHONY: clean
clean:
	rm -f profanity
	rm -f curses_example
