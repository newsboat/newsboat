#!/bin/sh

APPBASE_INFO=appbase.info
APPTEST_INFO=apptest.info
APPTOTAL_INFO=apptotal.info

make distclean
rm -rf $APPBASE_INFO $APPTEST_INFO html
make -j 5 PROFILE=1 all test
lcov --capture --initial --base-directory . --directory . --output-file $APPBASE_INFO
( cd test && ./test )
lcov --capture --base-directory . --directory . --output-file $APPTEST_INFO
lcov --base-directory . --directory . --output-file $APPTOTAL_INFO \
    --add-tracefile $APPBASE_INFO --add-tracefile $APPTEST_INFO
lcov --remove $APPTOTAL_INFO '/usr/*' --output-file $APPTOTAL_INFO
lcov --remove $APPTOTAL_INFO 'newsbeuter/test/*' --output-file $APPTOTAL_INFO
rm -rf html
genhtml -o html $APPTOTAL_INFO
