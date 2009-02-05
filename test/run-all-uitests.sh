#!/bin/sh

for f in *.rb ; do
	ruby "$f" 
	if [ $? != 0 ] ; then
		echo "Running test $f failed."
		#exit 1
	fi
done
