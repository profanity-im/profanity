CC = gcc
WARNS = -Werror -Wall -Wextra -Wno-unused-parameter -Wno-unused-but-set-variable
LIBS = -lxml2 -lssl -lresolv -lncurses -lstrophe
CFLAGS = -O3 $(WARNS) $(LIBS)
OBJS = log.o profanity.o

profanity: $(OBJS)
	$(CC) -o profanity $(OBJS) $(LIBS)

log.o: log.h
profanity.o: log.h

.PHONY: clean
clean:
	rm -f profanity
