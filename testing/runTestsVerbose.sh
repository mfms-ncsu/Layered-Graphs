#! /bin/bash
# runTestsVerbose.sh - Script for running regression tests - output shows all changes
#
#   @author Matt Stallmann
#   @date 2016/03/31

executable=../src/minimization
date=`date -u +"%F-%H%M"`
if ! [ -d History ]; then
    mkdir History
fi
output_file=$date.out
last_output=LastOutputs/last.out
diff_file=$date-diff.out

if [ ! -e $executable ] || [ ! -x $executable ]; then
    echo "$executable not found or not executable"
    exit
fi

echo "########## Testing minimization with sgf input, `date -u` ##########" >> $output_file
for sgf_file in TestData/*.sgf; do
    echo "### running experiments with $sgf_file, $date"
    echo "### running experiments with $sgf_file, $date" \
        >> $output_file
    echo "### $executable -p dfs -h bary -i 10000 -P b_t $sgf_file"
    echo "### $executable -p dfs -h bary -i 10000 -P b_t $sgf_file, $date" >> $output_file
    $executable -p dfs -h bary -i 10000 -P b_t $sgf_file >> $output_file 2>&1
    echo "" >> $output_file

    echo "### $executable -p dfs -h mod_bary -i 10000 -P b_t $sgf_file"
    echo "### $executable -p dfs -h mod_bary -i 10000 -P b_t $sgf_file, $date" >> $output_file
    $executable -p dfs -h mod_bary -i 10000 -P b_t $sgf_file >> $output_file 2>&1
    echo "" >> $output_file

    echo "### $executable -p dfs -h mce -i 10000 -P b_t $sgf_file"
    echo "### $executable -p dfs -h mce -i 10000 -P b_t $sgf_file, $date" >> $output_file
    $executable -p dfs -h mce -i 10000 -P b_t $sgf_file >> $output_file 2>&1
    echo "" >> $output_file

    echo "### $executable -p dfs -h sifting -i 10000 -P b_t $sgf_file"
    echo "### $executable -p dfs -h sifting -i 10000 -P b_t $sgf_file, $date" >> $output_file
    $executable -p dfs -h sifting -i 10000 -P b_t $sgf_file >> $output_file 2>&1
    echo "" >> $output_file

    echo "### $executable -p dfs -h mse -i 10000 -P t_s $sgf_file"
    echo "### $executable -p dfs -h mse -i 10000 -P t_s $sgf_file, $date" >> $output_file
    $executable -p dfs -h mse -i 10000 -P t_s $sgf_file >> $output_file 2>&1
    echo "" >> $output_file

    echo "### $executable -p dfs -h mce -i 10000 -P b_t -R 81453 $sgf_file"
    echo "### $executable -p dfs -h mce -i 10000 -P b_t -R 81453 $sgf_file, $date" >> $output_file
    $executable -p dfs -h mce -i 10000 -P b_t -R 81453 $sgf_file >> $output_file 2>&1
    echo "" >> $output_file

    echo "### $executable -p dfs -h mse -i 10000 -P t_s -R 81453 $sgf_file"
    echo "### $executable -p dfs -h mse -i 10000 -P t_s -R 81453 $sgf_file, $date" >> $output_file
    $executable -p dfs -h mse -i 10000 -P t_s -R 81453 $sgf_file >> $output_file 2>&1
    echo "" >> $output_file

    echo "===================================================" \
        >> $output_file
    echo "" >> $output_file
    echo "==================================================="
    echo
done

diff -bBw $last_output $output_file > $diff_file

# grep --invert-match "[Rr]untime" $last_output > $tmp_last
# grep --invert-match "[Rr]untime" $output_file > $tmp_next
# grep --invert-match "^###" $tmp_last | grep --invert-match "/src/minimization" > $tmp_last_nd
# grep --invert-match "^###" $tmp_next | grep --invert-match "/src/minimization" > $tmp_next_nd

# echo "-------- doing the diffs -----------"
# if [ $? -eq 0 ]; then
#     echo "No changes other than dates, keeping last output the same"
#     exit 0
# fi

more $diff_file
read

echo -n "Continue, i.e., no significant diffs (y/n)? "
read answer
if [ $answer = "y" ]; then
    cp $output_file $last_output
fi
