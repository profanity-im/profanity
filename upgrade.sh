#!/bin/sh

linux_upgrade()
{
    echo
    echo Profanity installer... upgrading Profanity
    echo
    ./bootstrap.sh
    ./configure
    make clean
    make
    sudo make install
    echo
    echo Profanity installer... upgrade complete!
    echo
    echo Type \'profanity\' to run.
    echo
}

cygwin_upgrade()
{
    echo
    echo Profanity installer... upgrading Profanity
    echo
    export LIBRARY_PATH=/usr/local/lib
    ./bootstrap.sh
    ./configure
    make clean
    make
    sudo make install
    echo
    echo Profanity installer... upgrade complete!
    echo
    echo Type \'profanity\' to run.
    echo
}

OS=`uname -s`
SYSTEM=unknown

if [ "${OS}" = "Linux" ]; then
    SYSTEM=linux
else
    echo $OS | grep -i cygwin
    if [ "$?" -eq 0 ]; then
        SYSTEM=cygwin
    fi
fi

case "$DIST" in
unknown)    echo The upgrade script will not work on this OS.
            echo Try a manual upgrade instead.
            exit
            ;;
linux)      fedora_prepare
            install_head_unit
            install_lib_strophe
            install_profanity
            ;;
cygwin)     debian_prepare
            install_head_unit
            install_lib_strophe
            install_profanity
            ;;
esac

