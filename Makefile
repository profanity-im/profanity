CC = gcc
CFLAGS = -O3 -Werror -Wall -Wextra -Wno-unused-parameter -Wno-unused-but-set-variable
LIBS = -lxml2 -lssl -lresolv -lncurses -lstrophe

profanity: clean
	$(CC) profanity.c $(LIBS) -o profanity 
	$(CC) roster.c $(LIBS) -o roster
	$(CC) active.c $(LIBS) -o active
	$(CC) basic.c $(LIBS) -o basic
	$(CC) bot.c $(LIBS) -o bot
	$(CC) curses_example.c -lncurses -o curses_example

.PHONY: clean
clean:
	rm -f profanity
	rm -f roster
	rm -f active
	rm -f basic
	rm -f bot
	rm -f curses_example
