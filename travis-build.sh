#!/bin/bash

error_handler()
{
    ERR_CODE=$?
    echo
    echo "Error $ERR_CODE with command '$BASH_COMMAND' on line ${BASH_LINENO[0]}. Exiting."
    echo
    exit $ERR_CODE
}

trap error_handler ERR

./bootstrap.sh

echo
echo "--> Building with ./configure --enable-notifications --enable-icons --enable-otr --enable-pgp --enable-plugins --enable-c-plugins --enable-python-plugins --with-xscreensaver"
echo
./configure --enable-notifications --enable-icons --enable-otr --enable-pgp --enable-plugins --enable-c-plugins --enable-python-plugins --with-xscreensaver
make
make check-unit
./profanity -v
make clean

echo
echo "--> Building with ./configure --disable-notifications --disable-icons --disable-otr --disable-pgp --disable-plugins --disable-c-plugins --disable-python-plugins --without-xscreensaver"
echo
./configure --disable-notifications --disable-icons --disable-otr --disable-pgp --disable-plugins --disable-c-plugins --disable-python-plugins --without-xscreensaver
make
make check-unit
./profanity -v
make clean

echo
echo "--> Building with ./configure --disable-notifications"
echo
./configure --disable-notifications
make
make check-unit
./profanity -v
make clean

echo
echo "--> Building with ./configure --disable-icons"
echo
./configure --disable-icons
make
make check-unit
./profanity -v
make clean

echo
echo "--> Building with ./configure --disable-otr"
echo
./configure --disable-otr
make
make check-unit
./profanity -v
make clean

echo
echo "--> Building with ./configure --disable-pgp"
echo
./configure --disable-pgp
make
make check-unit
./profanity -v
make clean

echo
echo "--> Building with ./configure --disable-pgp --disable-otr"
echo
./configure --disable-pgp --disable-otr
make
make check-unit
./profanity -v
make clean

echo
echo "--> Building with ./configure --disable-plugins"
echo
./configure --disable-plugins
make
make check-unit
./profanity -v
make clean

echo
echo "--> Building with ./configure --disable-python-plugins"
echo
./configure --disable-python-plugins
make
make check-unit
./profanity -v
make clean

echo
echo "--> Building with ./configure --disable-c-plugins"
echo
./configure --disable-c-plugins
make
make check-unit
./profanity -v
make clean

echo
echo "--> Building with ./configure --disable-c-plugins --disable-python-plugins"
echo
./configure --disable-c-plugins --disable-python-plugins
make
make check-unit
./profanity -v
make clean

echo
echo "--> Building with ./configure --without-xscreensaver"
echo
./configure --without-xscreensaver
make
make check-unit
./profanity -v
make clean

echo
echo "--> Building with ./configure"
echo
./configure
make
make check
./profanity -v
make clean

