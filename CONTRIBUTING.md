# Contributing to Profanity

## Coding style
Follow the style already present ;-)

To make this easier for you we created a `.clang-format` file.
You'll need to have `clang-format` installed.

Then just run `make format` before you do any commit.

It might be a good idea to add a git pre-commit hook.
So git automatically runs clang-format before doing a commit.

You can add the following snippet to `.git/hooks/pre-commit`:
```
for f in $(git diff --cached --name-only)
do
    clang-format -i $f
done
```

If you feel embarrassed every time the CI fails you can add the following
snippet to `.git/hooks/pre-push`:

```
#!/bin/sh
set -e
./ci-build.sh
```

This will run the same tests that the CI runs and refuse the push if it fails.
Note that it will run on the actual content of the repository directory and not
what may have been staged/committed.

If you're in a hurry you can add the `--no-verify` flag when issuing `git push`
and the `pre-push` hook will be skipped.

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
