#!/bin/sh
sed 's/||/\t/g' $1 > ${1/dsv/tsv}
gawk -f doc/CC2ListView/convertTable.awk ${1/dsv/tsv}
