#!/bin/sh
rm -f valgrind.out
#valgrind --log-file=valgrind.out --leak-check=full --track-origins=yes --show-reachable=yes ./profanity
valgrind --log-file=valgrind.out --leak-check=full --track-origins=yes ./profanity
