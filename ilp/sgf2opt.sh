#! /bin/bash

# starts with an sgf file and finds orderings that minimize two distinct objectives;
# four different optimizations are done:
#     - first objective alone
#     - second objective alone
#     - first given the optimum value of second as a constraint
#     - second given the optimum value of first as a constraint
#   The run for 'second given the optimum value of first as a constraint' is omitted
#   if 'first given the optimum value of second as a constraint' has the same outcome
#   as solving for first alone.
# (assumes sgf2ilp.py is in this directory and cplex_ilp is in PATH)

CPLEX_OPTIONS=""
CPLEX_TIME=3600

# echo to stderr
errcho() { >&2 echo $@; }

if [ $# -ne 3 ] && [ $# -ne 4 ]; then
    echo "Usage: $0 SGF_FILE OBJECTIVE_1 OBJECTIVE_2 [CPLEX_TIME]"
    echo "  where SGF_FILE is a file in .sgf format"
    echo "    and OBJECTIVE_1 and OBJECTIVE_2 are two layered graph objectives"
    echo "  CPLEX_TIME is the time limit for CPLEX, in seconds, default = 3600"
    echo "Finds, for the graph in SGF_FILE, minimum values for OBJECTIVE_1, OBJECTIVE_2,"
    echo " OBJECTIVE_1 with OBJECTIVE_2 restricted to its minimum value,"
    echo " and OBJECTIVE_2 with OBJECTIVE_1 restricted to its minimum value"
    echo "Possible objectives are to minimize ..."
    echo "  total/bottleneck (total/bottleneck crossings)"
    echo "  vertical/bn_vertical (minimize total/bottleneck non-verticality)"
    echo "  stretch/bn_stretch (total/bottleneck edge length with evenly spaced nodes)"
    echo "Produces files of the form, in the same directory as the input"
    echo "  FILE-TAG.lp (ILP for the appropriate problem)"
    echo "  FILE-TAG.out (the cplex output when the ILP is solved)"
    echo "FILE is the basename of the SGF_FILE"
    echo "TAG is either one of the objectives or, in case of the restricted run,"
    echo " x_v_y, where x is OBJECTIVE_1, v its optimal value, and y is OBJECTIVE_2"
    echo "Output gives an account of the cplex runs and the following information:"
    echo " [Objective-1 | Objective-2]                      	[the two objectives]"
    echo " [Value-1 | Value-2 | Value-1_2 | Value-2_1]      	[the relevant values of CPLEX runs]"
    echo "  here, 1_2 and 2_1 refer to objective 1 given objective 2 is minimum and vice versa"
    echo " [Status-1 | Status-2 | Status-1_2 Status-2_1]    	[status codes from the CPLEX runs]"
    echo " [Runtime-1 | Runtime-2 | Runtime-1_2 Runtime-2_1]	[runtimes of the CPLEX runs]"
    echo " Different                    	YES/no"
    echo "    YES if unresticted and restricted minima for objective 1 (and objective 2) differ"
    echo "    no if they are the same"
    exit 1
fi

# returns a (string representing a) floating point number that is slightly
# rounded up from the input (also a string); used for stretch objectives
# a 'p' is used instead of a decimal point to avoid having it look like an extension
# currently needed only for the stretch objective; all others are integers
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
# sets the variable runtime to the runtime of the CPLEX run (uses CPLEX internal clock)
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
        local cplex_creation_flags="--objective=$objective --$conditional_objective=$conditional_value"
    else
        local cplex_input_file=${base}-$objective.lp
        local cplex_output_file=${base}-$objective.out
        local cplex_creation_flags="--objective=$objective"
    fi

    errcho "_____________"
    if [ -e $cplex_input_file ]; then
        errcho " __ $cplex_input_file already exists, not creating"
    else
        errcho " __ creating ILP, objective is $objective, input file is $input_sgf_file"
        if [ "$conditional_objective" != "" ]; then
            errcho " __ conditional_objective is $conditional_objective, value = $conditional_value"
        fi
        $script_dir/sgf2ilp.py $cplex_creation_flags < $sgf_file > $cplex_input_file
    fi
    if [ -e $cplex_output_file ]; then
        errcho " __ already run CPLEX on $cplex_input_file, not repeating"
    else
        errcho -n " __ solving $cplex_input_file,      "
        date -u 1>&2
        cplex_ilp $CPLEX_OPTIONS -solution $cplex_input_file > $cplex_output_file
        errcho -n " __ done solving $cplex_input_file, "
        date -u 1>&2
    fi
    min_objective=`grep '^value' $cplex_output_file | cut -f 2`
    status=`grep '^StatusCode' $cplex_output_file | cut -f 2`
    runtime=`grep '^CPXtime' $cplex_output_file | cut -f 2`
    errcho " __ minimum value for $objective is $min_objective, status is $status"
}

# python scripts are in same directory as this one
script_dir=${0%/*}
input_sgf_file=$1
shift
objective_1=$1
shift
objective_2=$1
shift
cplex_time=$CPLEX_TIME
if [ $# -eq 1 ]; then
    cplex_time=$1
fi
CPLEX_OPTIONS="$CPLEX_OPTIONS -time=$cplex_time"

errcho "__ objective_1 = $objective_1, objective_2 = $objective_2"

# minimize the first objective
run_cplex $input_sgf_file $objective_1
value_1=$min_objective
status_1=$status
runtime_1=$runtime

# minimize the second objective
run_cplex $input_sgf_file $objective_2
value_2=$min_objective
status_2=$status
runtime_2=$runtime

# minimize first objective given minimum second objective as a constraint
run_cplex $input_sgf_file $objective_1 $objective_2 $value_2
value_1_2=$min_objective
status_1_2=$status
runtime_1_2=$runtime
errcho "__ minimum value for $objective_1 given $objective_2=$value_2 is $value_1_2, status is $status_1_2"

different="no"
if [ "$value_1" != "$value_1_2" ]; then
    different="YES"
    # minimize second objective given minimum first objective as a constraint
    run_cplex $input_sgf_file $objective_2 $objective_1 $value_1
    value_2_1=$min_objective
    status_2_1=$status
    runtime_2_1=$runtime
    errcho "__ minimum value for $objective_2 given $objective_1=$value_1 is $value_2_1, status is $status_2_1"
    errcho "__ *** the constrained minima differ from the unconstrained ones ***"
else
    # no need to do another run if objectives are the same
    value_2_1=$value_2
    status_2_1=$status_2
    runtime_2_1=0.0
fi


# tagged values for this run
echo "00-Instance	`basename $input_sgf_file .sgf`"
echo "Objective-1	$objective_1"
echo "Objective-2	$objective_2"
echo "Value-1    	$value_1"
echo "Value-1_2	$value_1_2"
echo "Value-2    	$value_2"
echo "Value-2_1	$value_2_1"
echo "Different 	$different"
echo "Status-1  	$status_1"
echo "Status-1_2	$status_1_2"
echo "Status-2  	$status_2"
echo "Status-2_1	$status_2_1"
echo "Runtime-1 	$runtime_1"
echo "Runtime-1_2	$runtime_1_2"
echo "Runtime-2 	$runtime_2"
echo "Runtime-2_1	$runtime_2_1"

echo "------------------------------------------"
echo
