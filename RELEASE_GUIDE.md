# Release Guide

* Release libstrophe if required

* Run Unit tests: `make check-unit`
* Run Functional tests - Currently disabled
* Run manual valgrind tests for new features
* Build and simple tests in Virtual machines ideally all dists including OSX and Windows (Cygwin)

* Update Inline command help (./src/command/cmd_defs.c)
* Check copyright dates in all files

* Generate HTML docs (the docgen argument only works when package status is development)
    `./profanity docgen`

* Generate manpages for profanity commands (the mangen argument only works when package status is development)
    `./profanity mangen`
  These files should be added to the docs subfolder and added to git whenever a command changes.

* Determine if libprofanitys version needs to be [increased](https://github.com/profanity-im/profanity/issues/973)
* Update plugin API docs (./apidocs/c and ./apidocs/python) need to run the `gen.sh` and commit the results to the website git repo
* Update CHANGELOG
* Update profrc.example
* Update profanity.doap (new XEPs and latest version)

## Creating artefacts
* Set the correct release version in configure.ac:

```
AC_INIT([profanity], [0.6.0], [boothj5web@gmail.com])
```

* Set the package status in configure.ac:

```
PACKAGE_STATUS="release"
```

* Add generated command manpages: `git add docs/profanity-*.1`

* Commit
* Tag (0.1.2)
* Push

* Configure to generate fresh Makefile:

```
./bootstrap.sh && ./configure
```

* Generate tarballs:

```
make dist
make dist-bzip2
make dist-xz
make dist-zip
```

* Set the package status back to dev:

```
PACKAGE_STATUS="development"
```

* Remove generated command manpages:
  `git rm docs/profanity-*.1`
  `git checkout HEAD -- docs/profanity-ox-setup.1`
  docs/profanity.1 and docs/profanity-ox-setup.1 are handwritten.

* Push

## Updating website
  * Make changes to the git repo including uploading the new artefacts at:
        https://github.com/profanity-im/profanity-im.github.io
  * Add .xz and .zip tarballs to `tarballs` directory
  * Copy `guide/latest` to `guide/newversion`
  * Update tarball location and name in index.html
  * Update checksums in index.html
  * Update profanity_version.txt
  * Take results from profanity.doap and put them into xeps.html
