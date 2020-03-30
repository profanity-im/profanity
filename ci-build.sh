#!/usr/bin/env bash

error_handler()
{
    ERR_CODE=$?
    echo
    echo "Error ${ERR_CODE} with command '${BASH_COMMAND}' on line ${BASH_LINENO[0]}. Exiting."
    echo
    exit ${ERR_CODE}
}

trap error_handler ERR

./bootstrap.sh

tests=()
MAKE="make --quiet -j$(nproc)"
CC="gcc"

case $(uname | tr '[:upper:]' '[:lower:]') in
    linux*)
        tests=(
        "--enable-notifications --enable-icons-and-clipboard --enable-otr --enable-pgp
        --enable-omemo --enable-plugins --enable-c-plugins
        --enable-python-plugins --with-xscreensaver"
        "--disable-notifications --disable-icons --disable-otr --disable-pgp
        --disable-omemo --disable-plugins --disable-c-plugins
        --disable-python-plugins --without-xscreensaver"
        "--disable-notifications"
        "--disable-icons"
        "--disable-otr"
        "--disable-pgp"
        "--disable-omemo"
        "--disable-pgp --disable-otr"
        "--disable-pgp --disable-otr --disable-omemo"
        "--disable-plugins"
        "--disable-python-plugins"
        "--disable-c-plugins"
        "--disable-c-plugins --disable-python-plugins"
        "--without-xscreensaver"
        "")
        ;;
    darwin*)
        tests=(
        "--enable-notifications --enable-icons-and-clipboard --enable-otr --enable-pgp
        --enable-omemo --enable-plugins --enable-c-plugins
        --enable-python-plugins"
        "--disable-notifications --disable-icons --disable-otr --disable-pgp
        --disable-omemo --disable-plugins --disable-c-plugins
        --disable-python-plugins"
        "--disable-notifications"
        "--disable-icons"
        "--disable-otr"
        "--disable-pgp"
        "--disable-omemo"
        "--disable-pgp --disable-otr"
        "--disable-pgp --disable-otr --disable-omemo"
        "--disable-plugins"
        "--disable-python-plugins"
        "--disable-c-plugins"
        "--disable-c-plugins --disable-python-plugins"
        "")
        ;;
    openbsd*)
        MAKE="gmake"
        # TODO(#1231):
        # `-std=gnu99 -fexec-charset=UTF-8` to silence:
        # src/event/server_events.c:1453:19: error: universal character names are only valid in C++ and C99
        # src/event/server_events.c:1454:19: error: universal character names are only valid in C++ and C99
        CC="gcc -std=gnu99 -fexec-charset=UTF-8"

        tests=(
        "--enable-notifications --enable-icons-and-clipboard --enable-otr --enable-pgp
        --enable-omemo --enable-plugins --enable-c-plugins
        --enable-python-plugins"
        "--disable-notifications --disable-icons --disable-otr --disable-pgp
        --disable-omemo --disable-plugins --disable-c-plugins
        --disable-python-plugins"
        "--disable-notifications"
        "--disable-icons"
        "--disable-otr"
        "--disable-pgp"
        "--disable-omemo"
        "--disable-pgp --disable-otr"
        "--disable-pgp --disable-otr --disable-omemo"
        "--disable-plugins"
        "--disable-python-plugins"
        "--disable-c-plugins"
        "--disable-c-plugins --disable-python-plugins"
        "")
        ;;
esac

for features in "${tests[@]}"
do
    echo
    echo "--> Building with ./configure ${features}"
    echo

    # shellcheck disable=SC2086
    ./configure $features

    $MAKE CC="${CC}"
    $MAKE check

    if [ $? -eq 1 ]; then
        cat ./test-suite.log
    fi

    ./profanity -v
    $MAKE clean
done
