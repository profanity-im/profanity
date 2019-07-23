# Contributing to Profanity

## Coding style
Follow the style already present ;-)

## Pull Requests
Before submitting a Pull Request please run valgrind and the clang static code analyzer.

### valgrind
We provide a suppressions file `prof.supp`. It is a combination of the suppressions for shipped with glib2, python and custom rules.

`G_DEBUG=gc-friendly G_SLICE=always-malloc valgrind --tool=memcheck --track-origins=yes --leak-check=full --leak-resolution=high --num-callers=30 --show-leak-kinds=definite --log-file=profval --suppressions=prof.supp ./profanity`

### clang

Running the clang static code analyzer helps improving the quality too.

```
make clean
scan-build make
scan-view ...
```
