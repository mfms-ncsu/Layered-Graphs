#! /bin/bash
# runSgfTests.sh - Script for running regression tests on minimization with sgf files
#
#   @author Matt Stallmann
#   @date 2016/03/31

executable=../src/minimization
date=`date -u +"%F-%H%M"`
if ! [ -d History ]; then
    mkdir History
fi
output_file=History/$date-sgf.out
last_output=TestOutputs/last.out
if [ ! -e $executable ] || [ ! -x $executable ]; then
    echo "$executable not found or not executable"
    exit
fi

echo "++++++++++ Testing minimization `date -u` ++++++++++++" >> $output_file
for sgf_file in TestData/*.sgf; do
    echo "========= running experiments with $sgf_file, $date ==============="
    echo "========= running experiments with $sgf_file, $date ===============" \
        >> $output_file
    echo "$executable -p dfs -h bary -i 10000 -P b_t -z $sgf_file"
    echo "$executable -p dfs -h bary -i 10000 -P b_t -z $sgf_file, $date" >> $output_file
    $executable -p dfs -h bary -i 10000 -P b_t -z $sgf_file >> $output_file 2>&1
    echo "" >> $output_file

    echo "$executable -p dfs -h mod_bary -i 10000 -P b_t -z $sgf_file"
    echo "$executable -p dfs -h mod_bary -i 10000 -P b_t -z $sgf_file, $date" >> $output_file
    $executable -p dfs -h mod_bary -i 10000 -P b_t -z $sgf_file >> $output_file 2>&1
    echo "" >> $output_file

    echo "$executable -p dfs -h mce -i 10000 -P b_t -z $sgf_file"
    echo "$executable -p dfs -h mce -i 10000 -P b_t -z $sgf_file, $date" >> $output_file
    $executable -p dfs -h mce -i 10000 -P b_t -z $sgf_file >> $output_file 2>&1
    echo "" >> $output_file

    echo "$executable -p dfs -h sifting -i 10000 -P b_t -z $sgf_file"
    echo "$executable -p dfs -h sifting -i 10000 -P b_t -z $sgf_file, $date" >> $output_file
    $executable -p dfs -h sifting -i 10000 -P b_t -z $sgf_file >> $output_file 2>&1
    echo "" >> $output_file

    echo "$executable -p dfs -h mse -i 10000 -P s_t -z $sgf_file"
    echo "$executable -p dfs -h mse -i 10000 -P s_t -z $sgf_file, $date" >> $output_file
    $executable -p dfs -h mse -i 10000 -P s_t -z $sgf_file >> $output_file 2>&1
    echo "" >> $output_file

    echo "$executable -p dfs -h mce -i 10000 -P b_t -z -R 81453 $sgf_file"
    echo "$executable -p dfs -h mce -i 10000 -P b_t -z -R 81453 $sgf_file, $date" >> $output_file
    $executable -p dfs -h mce -i 10000 -P b_t -z -R 81453 $sgf_file >> $output_file 2>&1
    echo "" >> $output_file

    echo "$executable -p dfs -h mse -i 10000 -P s_t -z -R 81453 $sgf_file"
    echo "$executable -p dfs -h mse -i 10000 -P s_t -z -R 81453 $sgf_file, $date" >> $output_file
    $executable -p dfs -h mse -i 10000 -P s_t -z -R 81453 $sgf_file >> $output_file 2>&1
    echo "" >> $output_file

    echo "===================================================" \
        >> $output_file
    echo "" >> $output_file
    echo "==================================================="
    echo
done

tmp_last=/tmp/$$_last_sol
tmp_next=/tmp/$$_next_sol
grep --invert-match "[Rr]untime" $last_output > $tmp_last
grep --invert-match "[Rr]untime" $output_file > $tmp_next

echo "-------- doing the diff -----------"
diff -bB $tmp_last $tmp_next

echo -n "Continue (y/n)? "
read answer
if [ $answer = "y" ]; then
    cp $output_file $last_output
    rm $tmp_last $tmp_next
fi

#  [Last modified: 2021 01 02 at 22:38:10 GMT]
