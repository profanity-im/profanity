all: test-c-plugin.so

%.so:%.o
	$(CC) -shared -fpic -lprofanity -o $@ $^

%.o:%.c
	$(CC) $(INCLUDE) -D_GNU_SOURCE -D_BSD_SOURCE -fpic -O3 -std=c99 \
        -Wall -Wextra -lprofanity -pedantic -c -o $@ $<

clean:
	$(RM) test-c-plugin.so
