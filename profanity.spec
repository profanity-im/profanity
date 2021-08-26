Name:		profanity
Version:	0.8.1
Release:	2%{?dist}
Summary:	A console based XMPP client

Group:		Application/System
License:	GPL
URL:		https://profanity-im.github.io/
Source0:	%{name}-%{version}.tar.gz

BuildRequires:	libstrophe-devel
# or:
#BuildRequires:	libmesode-devel
BuildRequires:	libcurl-devel
BuildRequires:	ncurses-devel
BuildRequires:	openssl-devel
BuildRequires:	glib2-devel >= 2.56.0
BuildRequires:	expat-devel
BuildRequires:	libotr-devel
BuildRequires:	gnutls-devel
BuildRequires:	sqlite3-devel >= 3.27.0
BuildRequires:	libsignal-protocol-c-devel >= 2.3.2
Requires:	libstrophe
Requires:	libcurl
Requires:	ncurses-libs
Requires:	openssl
Requires:	glib2 >= 2.56.0
Requires:	expat
Requires:	libotr
Requires:	gnutls
Requires:	sqlite3-devel >= 3.27.0
Requires:	libsignal-protocol-c-devel >= 2.3.2

%description
Profanity is a console based XMPP client written in C using ncurses and libstrophe, inspired by Irssi.

%prep
%setup -q

%build
%configure
make %{?_smp_mflags}

%install
make install DESTDIR=%{buildroot}

%files
%{_bindir}/profanity
%doc %{_datadir}/man/man1/profanity.1.gz

%changelog
