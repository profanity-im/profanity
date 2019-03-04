# Release Guide

Usually release candidates are tagged 0.6.0.rc1, 0.6.0.rc2 and tested for a week or so.

* Release libstrophe and libmesode if required

* Run Unit tests: `make check-unit`
* Run Functional tests - Currently disabled
* Run manual valgrind tests for new features
* Build and simple tests in Virtual machines ideally all dists including OSX and Windows (Cygwin)

* Update Inline command help (./src/command/cmd_defs.c)
* Check copright dates in all files (Copywright 2012-2019)

* Generate HTML docs (the docgen argument only works when package status is development)
    `./profanity docgen`

* Update plugin API docs (./apidocs/c and ./apidocs/python) need to run the `gen.sh` and commit the results to the website git repo
* Update CHANGELOG
* Update profrc.example

## Creating artefacts
* Set the correct release version in configure.ac:

```
AC_INIT([profanity], [0.6.0], [boothj5web@gmail.com])
```

* Set the package status in configure.ac:

```
PACKAGE_STATUS="release"
```

* Commit
* Tag (0.6.0)
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

* Set version to next release:

```
AC_INIT([profanity], [0.7.0], [boothj5web@gmail.com])
```

* Set the pacakge status back to dev:

```
PACKAGE_STATUS="development"
```

* Create a branch for patch releases (0.6.patch)
* Push

## Updating website
  * Make changes to the git repo incuding uploading the new artefacts at:
        https://github.com/boothj5/www_profanity_im
  * Email boothj5web@gmail.com to get them published to the website
