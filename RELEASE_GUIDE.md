# Release Guide

* Release libstrophe if required

* Set the correct release version in meson.build

* Run Unit tests: `meson setup build_test -Dtests=true && meson test -C build_test`
* Run Functional tests: same as above but needs stabber installed

* Check copyright dates in all files

* Create clean build folder:
  `meson setup build_deb --buildtype=debug -Dnotifications=enabled -Dicons-and-clipboard=enabled -Dotr=enabled -Dpgp=enabled -Domemo=enabled -Dc-plugins=enabled -Dpython-plugins=enabled -Dxscreensaver=enabled -Domemo-qrcode=enabled -Dgdk-pixbuf=enabled`

* Build profanity: `meson compile -C build_deb`

* Generate HTML docs (the docgen argument only works when package status is development)
    `./build_deb/profanity docgen`

* Generate manpages (the mangen argument only works when package status is development)
    `./build_deb/profanity mangen`

* Determine if libprofanitys version needs to be [increased](https://github.com/profanity-im/profanity/issues/973)
* Update plugin API docs (./apidocs/c and ./apidocs/python) need to run the `gen.sh` and commit the results to the website git repo
* Update CHANGELOG (Use scripts/changelog-helper.py)
* Update profrc.example
* Update profanity.doap (new XEPs and latest version). Look for `DEV` which marks what is done on the development branch.
* Add new release to profanity.doap

* Set the package status to release: `meson setup build_rel --buildtype=release`

* Update date and version in man pages (profanity.1, profanity-ox-setup.1)
* Add generated command manpages: `git add docs/profanity-*.1`

* Commit (Release 0.1.2)
* Tag (0.1.2)

* Generate tarballs: `meson dist -C build_rel --formats xztar,zip`

* Remove generated command manpages:
  `git rm docs/profanity-*.1`
  `git checkout HEAD -- docs/profanity-ox-setup.1`
  docs/profanity.1 and docs/profanity-ox-setup.1 are handwritten.

* Commit `Start new cycle`
* Push

## Updating website
Make changes to the git repo including uploading the new artefacts at:
https://github.com/profanity-im/profanity-im.github.io

* Add .xz and .zip tarballs to `tarballs` directory
* Copy generated main_fragment.html and toc_fragment.html to `guide/latest`
* Copy `guide/latest` to `guide/newversion`
* Tarball location, name, checksum in index.html will be updated automatically during `make` / GH Action
* Update profanity_version.txt
* Take results from profanity.doap and put them into xeps.html
