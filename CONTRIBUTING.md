# Contributing to Profanity

## Submitting patches
We recommend for people to always work on a dedicated git branch for each fix or feature.
Don't work on master.
So that they can easily pull master and rebase their work if needed.

For fixing (reported) bugs we usually use `git checkout -b fix/issuenumber-somedescription`.
When working on a new feature we usually use `git checkout -b feature/optionalissuenumber-somedescription`.

However this is not a rule just a recommendation to keep an overview of things.
If your change isn't a bugfix or new feature you can also just use any branch name.

### GitHub
We would like to encourage people to use GitHub to create pull requests.
It makes it easy for us to review the patches, track WIP branches, organize branches with labels and milestones,
and help others to see what's being worked on.

Also see the blogpost [Contributing a Patch via GitHub](https://profanity-im.github.io/blog/post/contributing-a-patch-via-github/).

### E-Mail
In case GitHub is down or you can't use it for any other reason, you can send a patch to our [mailing list](https://lists.notraces.net/mailman/listinfo/profanity).

We recommend that you follow the workflow mentioned above.
And create your patch using the [`git-format-patch`](https://git-scm.com/docs/git-format-patch) tool: `git format-patch master --stdout > feature.patch`

### Rules

* When fixing a bug, describe it and how your patch fixes it.
* When fixing a reported issue add an `Fixes https://github.com/profanity-im/profanity/issues/23` in the commit body.
* When adding a new feature add a description of the feature and how it should be used (workflow).
* If your patch adds a new configuration option add this to the `profrc.example` file.
* If your patch adds a new theming option add this to the `theme_template` file.
* Each patch or pull request should only contain related modifications.
* Run the tests and code formatters before submitting (c.f. Chapter 'Check everything' of this README).

## Coding style
Follow the style already present ;-)

To make this easier for you we created a `.clang-format` file.
You'll need to have `clang-format` installed.

Then just run `make format` before you do any commit.

It might be a good idea to add a git pre-commit hook.
So git automatically runs clang-format before doing a commit.

You can add the following snippet to `.git/hooks/pre-commit`:
```shell
for f in $(git diff --cached --name-only)
do
    if [[ "$f" =~ \.(c|h)$ ]]; then
        clang-format -i $f
    fi
done
```

If you feel embarrassed every time the CI fails you can add the following
snippet to `.git/hooks/pre-push`:

```shell
#!/bin/sh
set -e
./ci-build.sh
```

This will run the same tests that the CI runs and refuse the push if it fails.
Note that it will run on the actual content of the repository directory and not
what may have been staged/committed.

If you're in a hurry you can add the `--no-verify` flag when issuing `git push`
and the `pre-push` hook will be skipped.

## Finding mistakes
Test your changes with the following tools to find mistakes.

### unit tests

Run `make check` to run the unit tests with your current configuration or `./ci-build.sh` to check with different switches passed to configure.

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

### Finding typos

We include a `.codespellrc` configuration file for `codespell` in the root directory.
Before committing it might make sense to run `codespell` to see if you made any typos.

You can run the `make spell` command for this.

### Check everything

`make doublecheck` will run the code formatter, spell checker and unit tests.

