#!/bin/sh

APPBASE_INFO=appbase.info
APPTEST_INFO=apptest.info
APPTOTAL_INFO=apptotal.info

make distclean
rm -rf $APPBASE_INFO $APPTEST_INFO html
make -j 2 -f Makefile.prof all test test-rss
lcov -c -i -b . -d . -o $APPBASE_INFO
export OFFLINE=1
( cd test
./test
./test-rss
./run-uitests-headless.sh )
lcov -c -b . -d . -o $APPTEST_INFO
lcov -b . -d . -a $APPBASE_INFO -a $APPTEST_INFO -o $APPTOTAL_INFO
rm -rf html
genhtml -o html $APPTOTAL_INFO
