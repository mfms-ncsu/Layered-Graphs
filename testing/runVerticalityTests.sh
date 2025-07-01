#! /bin/bash
# runVerticalityTests.sh - Script for running regression tests on verticality heuristics

executable=../src/minimization
date=`date -u +"%F_%H%M"`
output_file=History/$date-v.out
last_output=VerticalityOutputs/last-v.out

# modified output files:
# the first two remove all lines with reference to runtime
tmp_last=VerticalityOutputs/$$_last_sol
tmp_next=VerticalityOutputs/$$_next_sol
# the next also remove header lines containing dates and times
# output has been standardized so that these all begin with ###
# also removed are lines that begin with the executable,
# embedded in a comment at the start of execution, so beginning with ### is not an option
tmp_last_nd=$$_last_sol_nd
tmp_next_nd=$$_next_sol_nd

if [ ! -e $executable ] || [ ! -x $executable ]; then
    echo "$executable not found or not executable"
    exit
fi

echo "########## Testing minimization with sgf input, `date -u` ##########" >> $output_file
for sgf_file in TestData/*.sgf; do
    echo "### running experiments with $sgf_file, $date"
    echo "### running experiments with $sgf_file, $date" \
        >> $output_file
    echo "### $executable -h vertical_bary -i 10000 $sgf_file, $date"
    echo "### $executable -h vertical_bary -i 10000 $sgf_file, $date" >> $output_file
    $executable -h vertical_bary -i 10000 $sgf_file >> $output_file 2>&1
    echo "" >> $output_file

    echo "### $executable -h mnve -i 10000 $sgf_file, $date"
    echo "### $executable -h mnve -i 10000 $sgf_file, $date" >> $output_file
    $executable -h mnve -i 10000 $sgf_file >> $output_file 2>&1
    echo "" >> $output_file

    echo "===================================================" >> $output_file
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
if [ "$answer" = "y" ]; then
    cp $output_file $last_output
    rm $tmp_last $tmp_next $tmp_last_nd $tmp_next_nd
fi
