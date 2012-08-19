#!/bin/sh
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
