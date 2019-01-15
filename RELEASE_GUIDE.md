# Release Guide

Usually release candidates are tagged 0.6.0.rc1, 0.6.0.rc2 and tested for a week or so.

* Release libstrophe and libmesode if required

* Run Unit tests: `make check-unit`
* Run Functional tests - Currently disabled
* Run manual valgrind tests for new features
* Build and simple tests in Virtual machines ideally all dists including OSX and Windows (Cygwin)

* Update Inline help
* Check copright dates in all files (Copywright 2012-2019)

* Generate HTML docs (the docgen argument only works when package status is development)
    `./profanity docgen`

* Update plugin docs
* Update CHANGELOG
* Update profrc.example

## Creating artefacts
* Clone the repository
* Set the correct release version in configure.ac:

```
AC_INIT([profanity], [0.6.0], [boothj5web@gmail.com])
```

* Set the package status in configure ac:

```
PACKAGE_STATUS="release"
```

* Commit
* Tag (0.6.0)
* Push
* Set version to next release

```
AC_INIT([profanity], [0.7.0], [boothj5web@gmail.com])
```

* Set the pacakge status back to dev

```
PACKAGE_STATUS="development"
```

* Create a branch for patch releases (0.6.patch)
* Push
* Clone the repository into a fresh folder and checkout the tag

```
git clone https://github.com/boothj5/profanity.git profanity-0.6.0
cd profanity-0.6.0
git checkout 0.6.0
```

* Remove files not needed in the artefcat

```
rm -rf apidocs .git
rm CHANGELOG configure-debug configure-plugins .gitignore profanity.spec prof.supp README.md theme_template travis-build.sh .travis.yml
```

* Bootstrap the build

```
./bootstrap.sh
```

* Remove automake cache

```
rm -rf autom4te.cache
```

* Leave the folder and create the artefacts

```
cd ..
tar -zcvf profanity-0.6.0.tar.gz profanity-0.6.0
zip -r profanity-0.6.0.zip profanity-0.6.0
```

## Updating website
  * Make changes to the git repo incuding uploading the new artefacts at:
        https://github.com/boothj5/www_profanity_im
  * Email boothj5web@gmail.com to get them published to the website
