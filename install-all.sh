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
    sudo apt-get -y install git automake autoconf libssl-dev libexpat1-dev libncursesw5-dev libglib2.0-dev libnotify-dev libcurl3-dev libxss-dev libotr5-dev libreadline-dev libtool libgpgme11-dev uuid-dev

}

fedora_prepare()
{
    echo
    echo Profanity installer... installing dependencies
    echo

    sudo dnf -y install gcc git autoconf automake openssl-devel expat-devel ncurses-devel glib2-devel libnotify-devel libcurl-devel libXScrnSaver-devel libotr3-devel readline-devel libtool libuuid-devel gpgme-devel
}

opensuse_prepare()
{
    echo
    echo Profanity installer...installing dependencies
    echo
    sudo zypper -n in gcc git automake make autoconf libopenssl-devel expat libexpat-devel ncurses-devel glib2-devel libnotify-devel libcurl-devel libXScrnSaver-devel libotr-devel readline-devel libtool libuuid-devel libgpgme-devel
}

centos_prepare()
{
    echo
    echo Profanity installer...installing dependencies
    echo

    sudo yum -y install epel-release
    sudo yum -y install git
    sudo yum -y install gcc autoconf automake cmake
    sudo yum -y install openssl-devel expat-devel ncurses-devel glib2-devel libnotify-devel libcurl-devel libXScrnSaver-devel libotr-devel readline-devel libtool libuuid-devel gpgme-devel
}

cygwin_prepare()
{
    echo
    echo Profanity installer... installing dependencies
    echo

    wget https://raw.githubusercontent.com/transcode-open/apt-cyg/master/apt-cyg
    #wget --no-check-certificate https://raw.github.com/boothj5/apt-cyg/master/apt-cyg
    #wget http://apt-cyg.googlecode.com/svn/trunk/apt-cyg
    chmod +x apt-cyg
    mv apt-cyg /usr/local/bin/

    if [ -n "$CYG_MIRROR" ]; then
        apt-cyg -m $CYG_MIRROR install git make gcc-core m4 automake autoconf pkg-config openssl-devel libexpat-devel zlib-devel libncursesw-devel libglib2.0-devel libcurl-devel libidn-devel libssh2-devel libkrb5-devel openldap-devel libgcrypt-devel libreadline-devel libgpgme-devel libtool libuuid-devel libpcre-devel libisl10 libcloog-isl4
    else
        apt-cyg install git make gcc-core m4 automake autoconf pkg-config openssl-devel libexpat-devel zlib-devel libncursesw-devel libglib2.0-devel libcurl-devel libidn-devel libssh2-devel libkrb5-devel openldap-devel libgcrypt-devel libreadline-devel libgpgme-devel libtool libuuid-devel libpcre-devel libisl10 libcloog-isl4

    fi
}

install_lib_strophe()
{
    echo
    echo Profanity installer... installing libstrophe
    echo
    git clone git://github.com/strophe/libstrophe.git
    cd libstrophe
    git checkout 0.8.8
    ./bootstrap.sh
    ./configure --prefix=$1
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
    git checkout 6f612d4
    ./bootstrap.sh
    ./bootstrap.sh # second call seems to fix problem on cygwin
    ./configure --prefix=/usr
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
    elif [ -f /etc/centos-release ]; then
        DIST=centos
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
            install_lib_strophe /usr
            install_profanity
            ;;
debian)     debian_prepare
            install_lib_strophe /usr
            install_profanity
            ;;
opensuse)   opensuse_prepare
            install_lib_strophe /usr/local
            sudo /sbin/ldconfig
            install_profanity
            ;;
centos)     centos_prepare
            install_lib_strophe /usr
            sudo ldconfig
            install_profanity
            ;;
cygwin)     cygwin_prepare
            cyg_install_lib_strophe
            cyg_install_profanity
            ;;
esac

cleanup
