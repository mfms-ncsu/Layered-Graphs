#!/bin/bash

## dot+ord2sgf - creates an sgf file from a dot and an ord file
# Uses dot_and_ord_to_sgf in ../src

if [ $# != 2 ]; then
    echo "Usage $0 DOT_FILE_NAME ORD_FILE_NAME"
    echo "  converts the dot and ord file to an sgf file;"
    echo "  the sgf file has a basename based on the ord file"
    echo "  and will be in the current directory"
fi

dot_file_name=$1
ord_file_name=$2
script_directory=${0%/*}
base=`basename $ord_file_name .ord`
# in case the extension is .dot.ORD, as in the bigraph-crossings (SGB) generation software
base=`basename $base .dot.ORD`
executable=$script_directory/../src/dot_and_ord_to_sgf
echo "c converted by dot+ord2sgf, date/time = `date -u`" > $base.sgf
$executable $dot_file_name $ord_file_name >> $base.sgf
