#! /bin/bash

# sgf2opt - starts with an sgf file and finds orderings that minimize two
# distinct objectives; three different optimizations are done: first
# objective alone, second objective alone, and second given the optimum value
# of first as a constraint.

# assumes sgf2ilp.py and sol2sgf.py are in this directory

CPLEX_OPTIONS="-time=3600 -feasible=4 -vsel=n"

if [ $# -ne 3 ]; then
    echo "Usage: $0 OBJECTIVE_1 OBJECTIVE_2 SGF_FILE"
    echo " where OBJECTIVE_1 and OBJECTIVE_2 are two layered graph objectives"
    echo "Finds, for the graph in SGF_FILE, minimum values for OBJECTIVE_1, OBJECTIVE_2,"
    echo " and OBJECTIVE_2 with OBJECTIVE_1 restricted to its minimum value"
    echo "Possible objectives are to minimize ..."
    echo "  total/bottleneck (total/bottleneck crossings)"
    echo "  vertical/bn_vertical (minimize total/bottleneck non-verticality)"
    echo "  stretch/bn_stretch (total/bottleneck edge length with evenly spaced nodes)"
    echo "Produces files of the form, in the same directory as the input"
    echo "  FILE-TAG.lp (ILP for the appropriate problem)"
    echo "  FILE-TAG.out (the cplex output when the ILP is solved)"
    echo "  FILE-TAG.sgf, (the sgf file with the optimum order)"
    echo "If TAG is one of x = t, b, v, bv, s, bs"
    echo "  the files represent total/bottleneck crossings, no-verticality, stretch"
    echo "If TAG has the form xEk_y"
    echo "  then y is minimized given that the x has (minimum) value k"
    echo "Output gives an account of the cplex runs and the following information:"
    echo " [OBJECTIVE1 | Objective2]    	[the two objectives]"
    echo " [Value1 | Value2 | Value2E1] 	[the relevant values of CPLEX runs]"
    echo " [Status1 | Status2 | Status2E1	[status codes from the CPLEX runs]"
    echo " Different                    	YES/no"
    echo "      YES if the unresticted and restricted minima for OBJECTIVE_2 differ"
    echo " When Difference is YES, there are two additional lines for creating a summary"
    echo "--> SGF_FILE <--"
    echo "~~~ OBJECTIVE_2 value_2 OBJECTIVE_2/OBJECTIVE_1=value_1 conditional_value ~~~"
    exit 1
fi

# returns a (string representing a) floating point number that is slightly
# rounded up from the input (also a string); used for stretch objectives
# a 'p' is used instead of a decimal point to avoid having it look like an extension
round_up() {
    local number=$1
    local after_point=${number##*\.}  # digits after decimal point
    local number_of_digits_after_point=${#after_point}
    # create a string of having a number of 0's corresponding to the number
    # of digits after the decimal point followed by a 1 and turn it into a
    # decimal number to be added
    local number_of_zeros=$(( $number_of_digits_after_point - 1 ))
    local zeros=`head -c $number_of_zeros < /dev/zero | tr '\0' '0'`
    local to_be_added=0.${zeros}1
    local rounded_number=`echo $number + $to_be_added | bc`
    echo $rounded_number
}

# @return a string version of a value, by substituting a 'p' for a decimal point
# if necessary
string_version() {
    # replace decimal point with a 'p'
    local objective=$1
    local objective_string=${objective/\./p}
    echo $objective_string
}

# runs cplex on base.sgf with the given objective,
#  possibly conditioned on the value of a second objective
# sets the variable min_objective to the [optimal] value if CPLEX run does not crash
# sets the variable status to "Optimal" if CPLEX found the optimal solution
# Usage: run_cplex SGF_FILE OBJECTIVE [CONDITIONAL_OBJECTIVE CONDITIONAL_VALUE]
run_cplex() {
    local sgf_file=$1
    local base=${input_sgf_file%.sgf}
    shift
    local objective=$1
    shift
    # handle situation where one objective is minimized given a value for another
    if [ $# -gt 0 ]; then
        local conditional_objective=$1
        local conditional_value=$2
        local suffix=${conditional_objective}_$(string_version $conditional_value)_$objective
        local cplex_input_file=${base}-$suffix.lp
        local cplex_output_file=${base}-$suffix.out
        local sgf_output_file=${base}-$suffix.sgf
        local cplex_creation_flags="--objective=$objective --$conditional_objective=$conditional_value"
    else
        local cplex_input_file=${base}-$objective.lp
        local cplex_output_file=${base}-$objective.out
        local sgf_output_file=${base}-$objective.sgf
        local cplex_creation_flags="--objective=$objective"
    fi

    echo "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%"
    if [ -e $cplex_input_file ]; then
        echo "__ $cplex_input_file already exists, not creating"
    else
        echo "creating ILP, objective is $objective, input file is $input_sgf_file"
        if [ "$conditional_objective" != "" ]; then
            echo " conditional_objective is $conditional_objective, value = $conditional_value"
        fi
        $script_dir/sgf2ilp.py $cplex_creation_flags < $sgf_file > $cplex_input_file
    fi
    if [ -e $cplex_output_file ]; then
        echo "__ already run CPLEX on $cplex_input_file, not repeating"
    else
        echo -n "solving $cplex_input_file,      "
        date -u
        cplex_ilp $CPLEX_OPTIONS -solution $cplex_input_file > $cplex_output_file
        echo -n "done solving $cplex_input_file, "
        date -u
    fi
    min_objective=`fgrep value $cplex_output_file | cut -f 2`
    status=`fgrep StatusCode $cplex_output_file | cut -f 2`
    echo "minimum value for $objective is $min_objective, status is $status"
    if [ -n "$min_objective" ]; then
        echo "converting solution to sgf for optimal order"
        $script_dir/sol2sgf.py < $cplex_output_file > $sgf_output_file
        if [[ $min_objective != ${min_objective%.[0-9]*} ]]; then
            # floating point number - need to round up just in case
            min_objective=$(round_up $min_objective)
        fi
        if [ $status != "Optimal" ]; then
            echo "!!! solution not proved optimal !!!"
        fi
    else
        echo "!!! no solution found for $cplex_input_file !!!"
    fi
}

# python scripts are in same directory as this one
script_dir=${0%/*}
objective_1=$1
shift
objective_2=$1
shift
input_sgf_file=$1

echo "objective_1 = $objective_1, objective_2 = $objective_2"

# minimize the first objective
run_cplex $input_sgf_file $objective_1
min_first_objective=$min_objective
first_status=$status
echo "minimum value for $objective_1 is $min_first_objective, status is $first_status"

# minimize second objective given minimum first objective as a constraint
run_cplex $input_sgf_file $objective_2 $objective_1 $min_objective
min_conditional_objective=$min_objective
conditional_status=$status
echo "minimum value for $objective_2 given $objective_1=$min_first_objective is $min_conditional_objective, status is $conditional_status"

# minimize the second objective
run_cplex $input_sgf_file $objective_2
min_second_objective=$min_objective
second_status=$status
echo "minimum value for $objective_2 is $min_second_objective, status is $second_status"

different="no"
if [ "$min_second_objective" != "$min_conditional_objective" ]; then
    different="YES"
fi

# tagged values for this run
echo "Input     	`basename $input_sgf_file .sgf`"
echo "Objective1	$objective_1"
echo "Objective2	$objective_2"
echo "Value1    	$min_first_objective"
echo "Value2    	$min_second_objective"
echo "Value2E$min_first_objective	$min_conditional_objective"
echo "Status1   	$first_status"
echo "Status2   	$second_status"
echo "Status2E$min_first_objective	$conditional_status"
echo "Different 	$different"

# compare these as strings because bash does not handle floats
if [ "$min_second_objective" != "$min_conditional_objective" ]; then
    echo "*-*-* $input_sgf_file objectives differ *-*-*"
    echo "~~~ $objective_2 $min_second_objective $objective_2/$objective_1=$min_first_objective $min_conditional_objective ~~~"
fi
echo "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%"
echo

#  [Last modified: 2020 05 20 at 15:21:23 GMT]
