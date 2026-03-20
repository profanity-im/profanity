# Scripts

This directory contains various scripts used for development, CI, and maintenance.

## `lint-and-test.sh`
Central validation script mainly for local development.

- **Purpose:** Runs spelling checks, code formatting, and unit testing.
- **Usage:** `./scripts/lint-and-test.sh [options]`
- **Options:**
  - `--fix-formatting`: Automatically apply `clang-format` fixes.
  - `--autotools` / `--meson`: Run unit tests using the specified build system.
  - `--hook`: Run in "hook mode" (checks only staged files).
  - `--install`: Install the script as a git `pre-commit` hook.

## `build-configuration-matrix.sh`
Exhaustive build configuration matrix testing. It ensures that Profanity compiles and passes unit tests across a variety of feature configurations for both Autotools and Meson.

- **Purpose:** Verifies architectural compatibility by testing many combinations of build flags.
- **Usage:** `./scripts/build-configuration-matrix.sh [autotools|meson] [extra-args]`
- **Extra Arguments:** Any arguments passed after the build system are forwarded directly to the configuration command (`./configure` or `meson setup`).
- **Environment:** Primarily used in CI (GitHub Actions), but can be run locally to verify all configurations.

## `changelog-helper.py`
Generates a sorted changelog from git commits since the last tag.

- **Purpose:** Automates the creation of release notes by parsing Conventional Commit messages.
- **Usage:** `python3 scripts/changelog-helper.py [--pr]`
- **Options:** Use `--pr` to append Pull Request numbers to each entry.

## `check-new-xeps.py`
Checks for updates to XMPP Extension Protocols (XEPs).

- **Purpose:** Compares the versions of XEPs implemented in Profanity (tracked in `profanity.doap`) against the latest versions available at `xmpp.org`.
- **Usage:** `python3 scripts/check-new-xeps.py`
