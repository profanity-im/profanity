Name:           profanity
Version:        0.18.1
Release:        %{autorelease}
Summary:        A console based XMPP client

License:        GPL-3.0-or-later WITH cryptsetup-OpenSSL-exception
URL:            https://profanity-im.github.io/
Source0:        %{url}/tarballs/%{name}-%{version}.tar.xz

BuildRequires:  gcc
BuildRequires:  meson
# Base:
BuildRequires:  libstrophe-devel
BuildRequires:  ncurses-devel
BuildRequires:  glib2-devel
BuildRequires:  libcurl-devel
BuildRequires:  readline-devel
BuildRequires:  sqlite-devel
BuildRequires:  python-unversioned-command
# Optional dependencies for support:
# Desktop notification support
BuildRequires:  libnotify-devel
# OTR support
BuildRequires:  libotr-devel
# PGP support
BuildRequires:  gpgme-devel
# OMEMO support
BuildRequires:  libsignal-protocol-c-devel
# OMEMO support (>= 1.7)
BuildRequires:  libgcrypt-devel
# Python plugin support
BuildRequires:  python3-devel
# Support for display OMEMO QR code
BuildRequires:  qrencode-devel
# Spell check support
BuildRequires:  enchant2-devel
# Image loading
BuildRequires:  gdk-pixbuf2-devel
# Icons
BuildRequires:  gtk3-devel
# Screensaver, does not provide pkgconfig file
BuildRequires:  libXScrnSaver-devel
# For tests:
BuildRequires:  libcmocka-devel
# For docs:
BuildRequires:  doxygen
BuildRequires:  python3dist(docutils)
BuildRequires:  python3dist(sphinx)
BuildRequires:  texinfo

%description
Profanity is a console based XMPP client written in C using ncurses
and libstrophe, inspired by Irssi.



%package libs
Summary:       The shared libraries required for plugins of Profanity
Requires:       %{name}%{?_isa} = %{version}-%{release}

%description libs
The %{name}-libs package provides the essential shared libraries for any
plugin of Profanity written in C.

See: https://profanity-im.github.io/plugins.html



%package        devel
Summary:        Development files for libraries used by plugins of Profanity
Requires:       %{name}-libs%{?_isa} = %{version}-%{release}

%description    devel
The %{name}-devel package contains libraries and header files for
developing plugins written in C for Profanity.



%package        doc
Summary:        Documentation for %{name}
BuildArch:      noarch
Requires:       %{name} = %{version}-%{release}

%description    doc
The %{name}-doc package contains docbook documentation for developing
applications that use %{name}.



%prep
%autosetup
# Output docbook instead of HTML
sed -i "s/GENERATE_HTML          = YES/GENERATE_HTML          = NO/g" apidocs/c/c-prof.conf
sed -i "s/GENERATE_DOCBOOK       = NO/GENERATE_DOCBOOK       = YES/g" apidocs/c/c-prof.conf
sed -i "s/DOCBOOK_PROGRAMLISTING = NO/DOCBOOK_PROGRAMLISTING = YES/g" apidocs/c/c-prof.conf

%build
%meson -Dnotifications=enabled \
       -Dpython-plugins=enabled \
       -Dc-plugins=enabled \
       -Dotr=enabled \
       -Dpgp=enabled \
       -Domemo=enabled \
       -Domemo-backend=libsignal \
       -Domemo-qrcode=enabled \
       -Dspellcheck=enabled \
       -Dicons-and-clipboard=enabled \
       -Dgdk-pixbuf=enabled \
       -Dxscreensaver=enabled \
       -Dtests=true
%meson_build

# Build HTML documentation
pushd apidocs/c/
doxygen c-prof.conf  # results are in apidocs/c/docbook/
popd
pushd apidocs/python/
sphinx-apidoc -f -o . src
make texinfo  # results are in apidocs/python/_build/texinfo
pushd _build
pushd texinfo
makeinfo --docbook ProfanityPythonPluginsAPI.texi
popd
popd
popd


%install
%meson_install
# Remove libprofanity.la generated
rm -f %{buildroot}%{_libdir}/libprofanity.la

# Install docbook documentation for the doc subpackage
mkdir -p %{buildroot}%{_datadir}/help/en/profanity/
for file in apidocs/c/docbook/*.xml;
do
  install -m644 ${file} \
    %{buildroot}%{_datadir}/help/en/profanity
done
install -m644 apidocs/python/_build/texinfo/ProfanityPythonPluginsAPI.xml \
  %{buildroot}%{_datadir}/help/en/profanity

# Install example config file
cp -a profrc.example %{buildroot}%{_datadir}/%{name}/


%check
%meson_test "unit tests"



%files
%license COPYING LICENSE.txt
%doc CHANGELOG README.md
%{_bindir}/%{name}
%{_mandir}/man1/%{name}.*
%{_mandir}/man1/%{name}-*
%{_datadir}/%{name}/
%dir %{_docdir}/profanity
%{_docdir}/profanity/profrc.example
%{_docdir}/profanity/theme_template

%files libs
%{_libdir}/libprofanity.so.0{,.*}


%files devel
%{_libdir}/libprofanity.so
%{_includedir}/profapi.h


%files doc
%dir  %{_datadir}/help/en
%lang(en) %{_datadir}/help/en/profanity



%changelog
%autochangelog
