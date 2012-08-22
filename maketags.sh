#!/bin/sh
rm -f tags
rm -f cscope.out
ctags -R .
cscope -R -b
