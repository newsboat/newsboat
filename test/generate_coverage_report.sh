#!/bin/sh

set -e

APPBASE_INFO=appbase.info
APPTEST_INFO=apptest.info
APPTOTAL_INFO=apptotal.info

rm -rf $APPBASE_INFO $APPTEST_INFO html
find -name '*.gcda' -print0 | xargs -0 rm --force
make -j 5 PROFILE=1 all test
lcov --capture --initial --base-directory . --directory . --output-file $APPBASE_INFO
( cd test && ./test "$@" )
lcov --capture --base-directory . --directory . --output-file $APPTEST_INFO
lcov --base-directory . --directory . --output-file $APPTOTAL_INFO \
    --add-tracefile $APPBASE_INFO --add-tracefile $APPTEST_INFO

# Removing info about shared libraries
lcov --remove $APPTOTAL_INFO '/usr/*' --output-file $APPTOTAL_INFO
# Removing info about shared libraries (on NixOS)
lcov --remove $APPTOTAL_INFO '/nix/store/*' --output-file $APPTOTAL_INFO
# Removing info about compiler internals
lcov --remove $APPTOTAL_INFO '?.?.?/*' --output-file $APPTOTAL_INFO
lcov --remove $APPTOTAL_INFO '?.?.??/*' --output-file $APPTOTAL_INFO
lcov --remove $APPTOTAL_INFO '?.??.?/*' --output-file $APPTOTAL_INFO
lcov --remove $APPTOTAL_INFO '?.??.??/*' --output-file $APPTOTAL_INFO
# Removing info about Newsboat's tests
lcov --remove $APPTOTAL_INFO 'newsboat/test/*' --output-file $APPTOTAL_INFO
lcov --remove $APPTOTAL_INFO '*/newsboat/test/*' --output-file $APPTOTAL_INFO
# Removing info about Newsboat's docs
lcov --remove $APPTOTAL_INFO 'newsboat/doc/*' --output-file $APPTOTAL_INFO
lcov --remove $APPTOTAL_INFO '*/newsboat/doc/*' --output-file $APPTOTAL_INFO
# Removing info about third-party libraries
lcov --remove $APPTOTAL_INFO '*/newsboat/3rd-party/*' --output-file $APPTOTAL_INFO

genhtml -o html $APPTOTAL_INFO
echo "The coverage report can be found at file://`pwd`/html/index.html"
