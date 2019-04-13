# Build the latest openSUSE Tumbleweed image
FROM opensuse/tumbleweed

RUN zypper --non-interactive in --no-recommends \
  git \
  gcc \
  autoconf \
  autoconf-archive \
  make \
  automake \
  libtool \
  glib2-devel \
  gtk2-devel \
  libXss-devel \
  libcurl-devel \
  libexpat-devel \
  libgpgme-devel \
  libmesode-devel \
  libnotify-devel \
  libotr-devel \
  libuuid-devel \
  libcmocka-devel \
  ncurses-devel \
  python3-devel \
  python3 \
  python-devel \
  python \
  readline-devel \
  libsignal-protocol-c-devel \
  libgcrypt-devel \
  libmicrohttpd-devel

RUN mkdir -p /usr/src
WORKDIR /usr/src

RUN mkdir -p /usr/src/stabber
RUN git clone git://github.com/boothj5/stabber.git
WORKDIR /usr/src/stabber
RUN ./bootstrap.sh
RUN ./configure --prefix=/usr --disable-dependency-tracking
RUN make
RUN make install

RUN mkdir -p /usr/src/profanity
WORKDIR /usr/src/profanity
COPY . /usr/src/profanity
