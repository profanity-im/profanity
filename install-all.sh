#!/bin/sh

ubuntu_deps()
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

DIST=unknown

uname -a | grep --ignore-case fedora
if [ $? -eq 0 ]; then
    DIST=fedora
fi

uname -a | grep --ignore-case ubuntu
if [ $? -eq 0 ]; then
    DIST=ubuntu
fi

case "$DIST" in
unknown)    echo Unsupported distribution, exiting.
            exit
            ;;
fedora)     fedora_deps
            ;;
ubuntu)     ubuntu_deps
            ;;
esac

install_head_unit
install_lib_strophe
install_profanity
cleanup
