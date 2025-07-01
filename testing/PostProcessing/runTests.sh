#! /bin/bash
# Lightweight script for running post processing tests

executable=../../src/minimization
date=`date -u +"%F-%H%M"`
bary_output="bary-out-$date"
mod_bary_output="mod_bary-out-$date"
sifting_output="sifting-out-$date"
mce_output="mce-out-$date"
mcn_output="mcn-out-$date"


if [ ! -e $executable ] || [ ! -x $executable ]; then
    echo "$executable not found or not executable"
    exit
fi

for sgf_file in ../TestData/*.sgf; do
    echo "### running experiments with $sgf_file, $date"

    echo "### $executable -p dfs -h bary -i 10000 -z -w _ $sgf_file"
    $executable -p dfs -h bary -i 10000 -z -w _ $sgf_file >> $bary_output 2>&1
    echo "" >> $bary_output

    echo "### $executable -p dfs -h mod_bary -i 10000 -z -w _ $sgf_file"
    $executable -p dfs -h mod_bary -i 10000 -z -w _ $sgf_file >> $mod_bary_output 2>&1
    echo "" >> $mod_bary_output

    echo "### $executable -p dfs -h mce -i 10000 -z -w _ $sgf_file"
    $executable -p dfs -h mce -i 10000 -z -w _ $sgf_file >> $mce_output 2>&1
    echo "" >> $mce_output

    echo "### $executable -p dfs -h sifting -i 10000 -z -w _ $sgf_file"
    $executable -p dfs -h sifting -i 10000 -z -w _ $sgf_file >> $sifting_output 2>&1
    echo "" >> $sifting_output

    echo "### $executable -p dfs -h mcn -i 10000 -z -w _ $sgf_file"
    $executable -p dfs -h mcn -i 10000 -z -w _ $sgf_file >> $mcn_output 2>&1
    echo "" >> $mcn_output
done