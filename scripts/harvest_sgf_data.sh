#! /bin/bash

# outputs lines of the form
#    TAG VALUE       (separated by tab)
# for tags LG-Nodes, LG-Edges, and LG-Layers,
# representing number of nodes, edges, and layers in the sgf input file

if [ $# -ne 1 ]; then
    echo "Usage: $0 FILE.sgf"
fi

file=$1

basename $file .sgf | awk '{printf "LG-Name\t%s\n", $1}'
grep '^n ' $file | wc | awk '{printf "LG-Nodes\t%d\n", $1}'
grep '^e ' $file | wc | awk '{printf "LG-Edges\t%d\n", $1}'
grep '^n ' $file | awk '{max = $3 > max ? $3 : max} END {printf "LG-Layers\t%d\n", max+1}'
grep '^n ' $file | awk '{w[$3]++} END {mw = w[0]; for (k in w) mw = w[k] < mw ? w[k] : mw; printf "LG-MinWidth\t%d\n", mw}'
grep '^n ' $file | awk '{w[$3]++} END {for (k in w) MW = w[k] > MW ? w[k] : MW; printf "LG-MaxWidth\t%d\n", MW}'
# unlikely for min degree to be greater than 1,000,000
grep '^e ' $file | awk '{d[$2]++; d[$3]++} END {md = 1000000; for (k in d) md = d[k] < md ? d[k] : md; printf "LG-MinDegree\t%d\n", md}'
grep '^e ' $file | awk '{d[$2]++; d[$3]++} END {for (k in d) MD = d[k] > MD ? d[k] : MD; printf "LG-MaxDegree\t%d\n", MD}'
