# Contributing to Profanity

## Build

Please follow the [build section](https://profanity-im.github.io/guide/latest/build.html) in our user guide.
You might also take a look at the `Dockerfile.*` in the root directory.

## Submitting patches
We recommend for people to always work on a dedicated git branch for each fix or feature.
Don't work on master.
So that they can easily pull master and rebase their work if needed.

For fixing (reported) bugs we usually use `git checkout -b fix/issuenumber-somedescription`.
When working on a new feature we usually use `git checkout -b feature/optionalissuenumber-somedescription`.

However this is not a rule just a recommendation to keep an overview of things.
If your change isn't a bugfix or new feature you can also just use any branch name.

### Commit messages
Write commit messages that make sense. Explain what and *why* you change.
Write in present tense.
Please give [this guideline](https://gist.github.com/robertpainsi/b632364184e70900af4ab688decf6f53) a read.

### GitHub
We would like to encourage people to use GitHub to create pull requests.
It makes it easy for us to review the patches, track WIP branches, organize branches with labels and milestones,
and help others to see what's being worked on.

Also see the blogpost [Contributing a Patch via GitHub](https://profanity-im.github.io/blog/post/contributing-a-patch-via-github/).

### E-Mail
In case GitHub is down or you can't use it for any other reason, you can send a patch to our [mailing list](https://lists.posteo.de/listinfo/profanity).

We recommend that you follow the workflow mentioned above.
And create your patch using the [`git-format-patch`](https://git-scm.com/docs/git-format-patch) tool: `git format-patch master --stdout > feature.patch`

### Another git service
We prefer if you create a pull request on GitHub.
Then our team can easily request reviews. And we have the history of the review saved in one place.

If using GitHub is out of the question but you are okay using another service (i.e.: GitLab, codeberg) then please message us in the MUC or send us an email.
We will then pull from your repository and merge manually.

### Rules

* When fixing a bug, describe it and how your patch fixes it.
* When fixing a reported issue add an `Fixes https://github.com/profanity-im/profanity/issues/23` in the commit body.
* When adding a new feature add a description of the feature and how it should be used (workflow).
* If your patch adds a new configuration option add this to the `profrc.example` file.
* If your patch adds a new theming option add this to the `theme_template` file.
* Each patch or pull request should only contain related modifications.
* Run the tests and code formatters before submitting (c.f. Chapter 'Check everything' of this README).
* When changing the UI it would be appreciated if you could add a before and after screenshot for comparison.
* Squash fixup commits into one
* If applicable, document how to test the functionality

## Hints and Pitfalls

* When adding a new hotkey/shortcut make sure it's not registered in Profanity already. And also that it's not a default shortcut of readline.
* We ship a `.git-blame-ignore-revs` file containing banal commits which you will most likely want to ignore when using `git blame`. In case you are using vim and [fugitive](https://github.com/tpope/vim-fugitive) `command Gblame Git blame --ignore-revs-file=.git-blame-ignore-revs` might be helpful in your vimrc. You can also set the `blame.ignoreRevsFile` option in your git config to have `git blame` generally ignore the listed commits.

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

*Note:* We provide a config file that describes our coding style for clang. But due to a mistake on their side it might happen that you can get a different result that what we expect. See [here](https://github.com/profanity-im/profanity/pull/1774) and [here](https://github.com/profanity-im/profanity/pull/1828) for details. We will try to always run latest clang-format.

In cases where you want to disable automatic formatting for a specific block of code (e.g. a complex lookup table), you can use the following comments:
```c
/* clang-format off */
// your code
/* clang-format on */
```

## Finding mistakes
Test your changes with the following tools to find mistakes.

### Unit tests

Run `make check` to run the unit tests with your current configuration or `./ci-build.sh` to check with different switches passed to configure.

### Writing unit tests

We use the [cmocka](https://cmocka.org/) testing framework for unit tests.

Test files are located in `tests/unittests` and should mirror the directory structure of the `src/` directory. For example, if you are testing `src/xmpp/jid.c`, the corresponding test file should be `tests/unittests/xmpp/test_jid.c`.

#### Naming Convention
Test functions must follow a behavior-driven naming convention using double underscores (`__`) as semantic separators:

`[unit]__[verb]__[scenario]`

- **unit**: The name of the function or module being tested.
- **verb**: A descriptive verb of the outcome (`returns`, `is`, `updates`, `shows`, `triggers`, `fails`).
- **scenario**: The specific condition or input being tested.

Examples:
- `jid_create__returns__null_from_null`
- `p_contact_is_available__is__false_when_offline`
- `cmd_connect__shows__usage_when_no_server_value`
- `str_replace__returns__one_substr`

#### Adding a new test
When adding a new test file:
1. Create the `.c` and `.h` files in the appropriate subdirectory of `tests/unittests`.
2. Register the test functions in `tests/unittests/unittests.c` within the `all_tests` array.
3. Add the new source file to `unittest_sources` in both `meson.build` and `Makefile.am`.

### valgrind
We provide a suppressions file `prof.supp`. It is a combination of the suppressions for shipped with glib2, python and custom rules.

`G_DEBUG=gc-friendly G_SLICE=always-malloc valgrind --tool=memcheck --track-origins=yes --leak-check=full --leak-resolution=high --num-callers=30 --show-leak-kinds=definite --log-file=profval --suppressions=prof.supp ./profanity`

There's also the option to create a "personalized" suppression file with the up-to-date glib2 and python suppressions.

`make my-prof.supp`

After executing this, you can replace the `--suppressions=prof.supp` argument in the above call, by `--suppressions=my-prof.supp`.

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

### Full check
Run the central quality check script before submitting a patch. This runs the spell checker, code formatter, and unit tests.

If using Autotools:
```
make doublecheck
```

If using Meson:
```
meson compile doublecheck
```

Alternatively, you can run the script directly:
```
./scripts/quality-check.sh --fix-formatting --meson  # or --autotools
```

### Pre-commit hook
It is highly recommended to install the quality check script as a git pre-commit hook. This will automatically check your staged changes for spelling and formatting issues every time you commit.

To install the hook:
```
./scripts/quality-check.sh --install
```

If you need to bypass the formatting check (e.g. due to a `clang-format` version mismatch), you can set the `SKIP_FORMAT` environment variable:
```
SKIP_FORMAT=1 git commit
```

