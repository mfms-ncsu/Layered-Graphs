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
output_file=History/$date.out
last_output=LastOutputs/last.out

# modified output files:
# the first two remove all lines with reference to runtime
tmp_last=/tmp/$$_last_sol
tmp_next=/tmp/$$_next_sol
# the next also remove header lines containing dates and times
# output has been standardized so that these all begin with ###
# also removed are lines that begin with the executable,
# embedded in a comment at the start of execution, so beginning with ### is not an option
tmp_last_nd=/tmp/$$_last_sol_nd
tmp_next_nd=/tmp/$$_next_sol_nd

if [ ! -e $executable ] || [ ! -x $executable ]; then
    echo "$executable not found or not executable"
    exit
fi

echo "########## Testing minimization with sgf input, `date -u` ##########" >> $output_file
for sgf_file in TestData/*.sgf; do
    echo "### running experiments with $sgf_file, $date"
    echo "### running experiments with $sgf_file, $date" \
        >> $output_file
    echo "### $executable -p dfs -h bary -i 10000 -P b_t -z $sgf_file"
    echo "### $executable -p dfs -h bary -i 10000 -P b_t -z $sgf_file, $date" >> $output_file
    $executable -p dfs -h bary -i 10000 -P b_t -z $sgf_file >> $output_file 2>&1
    echo "" >> $output_file

    echo "### $executable -p dfs -h mod_bary -i 10000 -P b_t -z $sgf_file"
    echo "### $executable -p dfs -h mod_bary -i 10000 -P b_t -z $sgf_file, $date" >> $output_file
    $executable -p dfs -h mod_bary -i 10000 -P b_t -z $sgf_file >> $output_file 2>&1
    echo "" >> $output_file

    echo "### $executable -p dfs -h mce -i 10000 -P b_t -z $sgf_file"
    echo "### $executable -p dfs -h mce -i 10000 -P b_t -z $sgf_file, $date" >> $output_file
    $executable -p dfs -h mce -i 10000 -P b_t -z $sgf_file >> $output_file 2>&1
    echo "" >> $output_file

    echo "### $executable -p dfs -h sifting -i 10000 -P b_t -z $sgf_file"
    echo "### $executable -p dfs -h sifting -i 10000 -P b_t -z $sgf_file, $date" >> $output_file
    $executable -p dfs -h sifting -i 10000 -P b_t -z $sgf_file >> $output_file 2>&1
    echo "" >> $output_file

    echo "### $executable -p dfs -h mse -i 10000 -P s_t -z $sgf_file"
    echo "### $executable -p dfs -h mse -i 10000 -P s_t -z $sgf_file, $date" >> $output_file
    $executable -p dfs -h mse -i 10000 -P s_t -z $sgf_file >> $output_file 2>&1
    echo "" >> $output_file

    echo "### $executable -p dfs -h mce -i 10000 -P b_t -z -R 81453 $sgf_file"
    echo "### $executable -p dfs -h mce -i 10000 -P b_t -z -R 81453 $sgf_file, $date" >> $output_file
    $executable -p dfs -h mce -i 10000 -P b_t -z -R 81453 $sgf_file >> $output_file 2>&1
    echo "" >> $output_file

    echo "### $executable -p dfs -h mse -i 10000 -P s_t -z -R 81453 $sgf_file"
    echo "### $executable -p dfs -h mse -i 10000 -P s_t -z -R 81453 $sgf_file, $date" >> $output_file
    $executable -p dfs -h mse -i 10000 -P s_t -z -R 81453 $sgf_file >> $output_file 2>&1
    echo "" >> $output_file

    echo "===================================================" \
        >> $output_file
    echo "" >> $output_file
    echo "==================================================="
    echo
done

grep --invert-match "[Rr]untime" $last_output > $tmp_last
grep --invert-match "[Rr]untime" $output_file > $tmp_next
grep --invert-match "^###" $tmp_last | grep --invert-match "/src/minimization" > $tmp_last_nd
grep --invert-match "^###" $tmp_next | grep --invert-match "/src/minimization" > $tmp_next_nd

echo "-------- doing the diffs -----------"
diff -bBw $tmp_last_nd $tmp_next_nd
if [ $? -eq 0 ]; then
    echo "No changes other than dates, keeping last output the same"
    exit 0
fi

echo -n "Outputs do not match; continue anyway (y/n)? "
read answer
if [ $answer = "y" ]; then
    cp $output_file $last_output
    rm $tmp_last $tmp_next $tmp_last_nd $tmp_next_nd
fi
