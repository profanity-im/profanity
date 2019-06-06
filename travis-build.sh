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

tests=()
case $(uname | tr '[:upper:]' '[:lower:]') in
    linux*)
        tests=(
        "--enable-notifications --enable-icons --enable-otr --enable-pgp
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
        "--enable-notifications --enable-icons --enable-otr --enable-pgp
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

for flags in "${tests[@]}"
do
    echo
    echo "--> Building with ./configure $flags"
    echo
    # shellcheck disable=SC2086
    ./configure $flags
    make
    ./profanity -v
    make clean

    echo "$flags"
done
