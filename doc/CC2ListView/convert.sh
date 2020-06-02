#!/bin/bash

if [ $# -eq 1 ]
then
	sed 's/||/\t/g' $1 > ${1/dsv/tsv}
	awk -f doc/CC2ListView/convertTable.awk ${1/dsv/tsv}
else
        echo "USAGE: $0 <dsv-file>"
fi
