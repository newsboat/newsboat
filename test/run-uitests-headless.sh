#!/bin/sh

rm -f *.rb.log

for f in *.rb ; do
	echo "Running $f..."
	ruby1.8 "$f" > /dev/null
	if [ $? != 0 ] ; then
		echo "Running test $f failed."
		#exit 1
	fi
done
