
all: test-c-plugin.so

%.so:%.o
	$(CC) -shared -fpic -o $@ $^

%.o:%.c
	$(CC) $(INCLUDE) -D_GNU_SOURCE -D_BSD_SOURCE -fpic -O3 -std=c99 \
        -Wall -Wextra -pedantic -c -o $@ $<

clean:
	$(RM) test-c-plugin.so
