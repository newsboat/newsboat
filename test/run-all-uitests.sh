#!/bin/sh

for f in *.rb ; do
	ruby "$f" || ( echo "Running test $f failed." ; exit 1 )
done
