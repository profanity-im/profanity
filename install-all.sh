#!/bin/sh

debian_deps()
{
    echo
    echo Profanity installer ... updating apt repositories
    echo
    sudo apt-get update

    echo
    echo Profanity installer... installing dependencies
    echo
    sudo apt-get -y install g++ autoconf libssl-dev libexpat1-dev libncurses5-dev libxml2-dev libglib2.0-dev libnotify-dev libcurl3-dev

}

fedora_deps()
{
    echo
    echo Profanity installer... installing dependencies
    echo

    ARCH=`arch`
    
    sudo yum -y install gcc gcc-c++ autoconf automake openssl-devel.$ARCH expat-devel.$ARCH ncurses-devel.$ARCH libxml2-devel.$ARCH glib2-devel.$ARCH libnotify-devel.$ARCH libcurl-devel.$ARCH
}

install_head_unit()
{
    echo
    echo Profanity installer... installing head-unit
    echo
    git clone git://github.com/boothj5/head-unit.git
    cd head-unit
    make
    sudo make install

    cd ..
}

install_lib_strophe()
{
    echo
    echo Profanity installer... installing libstrophe
    echo
    git clone git://github.com/metajack/libstrophe.git
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
    ./bootstrap.sh
    ./configure
    make
    sudo make install
}

cleanup()
{
    echo
    echo Profanity installer... cleaning up
    echo

    echo Removing head-unit repository...
    rm -rf head-unit

    echo Removing libstrophe repository...
    rm -rf libstrophe

    echo
    echo Profanity installer... complete!
    echo
    echo Type \'profanity\' to run.
    echo
}

OS=`uname -s`
DIST=unknown

if [ "${OS}" = "Linux" ]; then
    if [ -f /etc/fedora-release ]; then
        DIST=fedora
    elif [ -f /etc/debian-release ]; then
        DIST=debian
    fi
fi

case "$DIST" in
unknown)    echo The install script will not work on this OS.
            echo Try a manual install instead.
            exit
            ;;
fedora)     fedora_deps
            ;;
debian)     debian_deps
            ;;
esac

install_head_unit
install_lib_strophe
install_profanity
cleanup
