#!/bin/bash

set -o errtrace

STATUS=development

error_handler()
{
        ERR_CODE=$?
        echo "Error $ERR_CODE with command '$BASH_COMMAND' on line ${BASH_LINENO[0]}. Exiting."
        exit $ERR_CODE

}

trap error_handler ERR

debian_prepare()
{
    echo
    echo Profanity installer ... updating apt repositories
    echo
    sudo apt-get update

    echo
    echo Profanity installer... installing dependencies
    echo
    sudo apt-get -y install git automake autoconf libssl-dev libexpat1-dev libncursesw5-dev libglib2.0-dev libnotify-dev libcurl3-dev libxss-dev libotr2-dev

}

fedora_prepare()
{
    echo
    echo Profanity installer... installing dependencies
    echo

    ARCH=`arch`

    sudo yum -y install gcc git autoconf automake openssl-devel.$ARCH expat-devel.$ARCH ncurses-devel.$ARCH  glib2-devel.$ARCH libnotify-devel.$ARCH libcurl-devel.$ARCH libXScrnSaver-devel.$ARCH libotr3-devel.$ARCH
}

opensuse_prepare()
{
 echo
 echo Profanity installer...installing dependencies
 echo
 sudo zypper -n in gcc git automake make autoconf libopenssl-devel expat libexpat-devel ncurses-devel glib2-devel libnotify-devel libcurl-devel libXScrnSaver-devel libotr-devel
}

cygwin_prepare()
{
    echo
    echo Profanity installer... installing dependencies
    echo

    wget --no-check-certificate https://raw.github.com/boothj5/apt-cyg/master/apt-cyg
    #wget http://apt-cyg.googlecode.com/svn/trunk/apt-cyg
    chmod +x apt-cyg
    mv apt-cyg /usr/local/bin/

    if [ -n "$CYG_MIRROR" ]; then
        apt-cyg -m $CYG_MIRROR install git make gcc-core m4 automake autoconf pkg-config openssl-devel libexpat-devel zlib-devel libncursesw-devel libglib2.0-devel libcurl-devel libidn-devel libssh2-devel libkrb5-devel openldap-devel libgcrypt-devel
    else
        apt-cyg install git make gcc-core m4 automake autoconf pkg-config openssl-devel libexpat-devel zlib-devel libncursesw-devel libglib2.0-devel libcurl-devel libidn-devel libssh2-devel libkrb5-devel openldap-devel libgcrypt-devel

    fi

    export LIBRARY_PATH=/usr/local/lib/
}

install_lib_strophe()
{
    echo
    echo Profanity installer... installing libstrophe
    echo
    git clone git://github.com/strophe/libstrophe.git
    cd libstrophe
    ./bootstrap.sh
    ./configure
    make
    sudo make install

    cd ..
}

install_profanity()
{
    echo
    echo Profanity installer... installing Profanity
    echo
    if [ "${STATUS}" = "development" ]; then
        ./bootstrap.sh
    fi
    ./configure
    make
    sudo make install
}

cyg_install_lib_strophe()
{
    echo
    echo Profanity installer... installing libstrophe
    echo
    git clone git://github.com/strophe/libstrophe.git
    cd libstrophe
    ./bootstrap.sh
    ./bootstrap.sh # second call seems to fix problem on cygwin
    ./configure
    make
    make install

    cd ..
}

cyg_install_profanity()
{
    echo
    echo Profanity installer... installing Profanity
    echo
    if [ "${STATUS}" = "development" ]; then
        ./bootstrap.sh
    fi
    ./configure
    make
    make install
}

cleanup()
{
    echo
    echo Profanity installer... cleaning up
    echo

    echo Removing libstrophe repository...
    rm -rf libstrophe

    echo
    echo Profanity installer... complete!
    echo
    echo Type \'profanity\' to run.
    echo
}

while getopts m: opt
do
    case "$opt" in
        m) CYG_MIRROR=$OPTARG;;
    esac
done

OS=`uname -s`
DIST=unknown

if [ "${OS}" = "Linux" ]; then
    if [ -f /etc/fedora-release ]; then
        DIST=fedora
    elif [ -f /etc/debian_version ]; then
        DIST=debian
    elif [ -f /etc/os-release ]; then
        DIST=opensuse
    fi
else
    echo $OS | grep -i cygwin
    if [ "$?" -eq 0 ]; then
        DIST=cygwin
    fi
fi

case "$DIST" in
unknown)    echo The install script will not work on this OS.
            echo Try a manual install instead.
            exit
            ;;
fedora)     fedora_prepare
            install_lib_strophe
            install_profanity
            ;;
debian)     debian_prepare
            install_lib_strophe
            install_profanity
            ;;
opensuse)   opensuse_prepare
            install_lib_strophe
            install_profanity
            ;;
cygwin)     cygwin_prepare
            cyg_install_lib_strophe
            cyg_install_profanity
            ;;
esac

cleanup
