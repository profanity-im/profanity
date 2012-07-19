#!/bin/sh

echo
echo Profanity installer ... updating apt repositories
echo
sudo apt-get update

echo
echo Profanity installer... installing dependencies
echo
sudo apt-get -y install g++ autoconf libssl-dev libexpat1-dev libncurses5-dev libxml2-dev libglib2.0-dev libnotify-dev

echo
echo Profanity installer... installing head-unit
echo
git clone git://github.com/boothj5/head-unit.git
cd head-unit
make
sudo make install

cd ..

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

echo
echo Profanity installer... installing Profanity
echo
./bootstrap.sh
./configure
make
sudo make install

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
