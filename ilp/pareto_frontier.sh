#! /bin/bash

# starts with an sgf file and two extreme values 'low' and 'high' for one of two objectives
# for every value 'val' in the range [low,high]:
#     - compute the value of the other objective with the first constrained to be 'val'
# (assumes sgf2ilp.py is in this directory and cplex_ilp is in PATH)

# echo to stderr
errcho() { >&2 echo $@; }

CPLEX_OPTIONS=""
CPLEX_TIME=3600

if [ $# -ne 5 ] && [ $# -ne 6 ]; then
    echo "Usage: $0 SGF_FILE OBJECTIVE_1 OBJECTIVE_2 LOW HIGH [CPLEX_TIME]"
    echo " where OBJECTIVE_1 and OBJECTIVE_2 are two layered graph objectives"
    echo "   and LOW and HIGH are two values for OBJECTIVE_2"
    echo "Finds, for the graph in SGF_FILE, minimum values for OBJECTIVE_1"
    echo " with OBJECTIVE_2 constrained to be every integer value in range [LOW,HIGH]"
    echo "Possible objectives are to minimize ..."
    echo "  total/bottleneck (total/bottleneck crossings)"
    echo "  vertical/bn_vertical (minimize total/bottleneck non-verticality)"
    echo "  stretch/bn_stretch (total/bottleneck edge length with evenly spaced nodes)"
    echo "Produces files of the form, in the same directory as the input"
    echo "  FILE-TAG.lp (ILP for the appropriate problem)"
    echo "  FILE-TAG.out (the cplex output when the ILP is solved)"
    echo "TAG is x_v_y, where x is OBJECTIVE_2, y is OBJECTIVE_1,"
    echo "              and v is value of OBJECTIVE_2 used to constrain minimization of OBJECTIVE_1"
    echo "Output has the following tag-value lines for each CPLEX run:"
    echo " 00-Instance  	[the problem instance, i.e., base name of the file]"
    echo " [OBJECTIVE_2]	[value of OBJECTIVE_2, an integer in the range [LOW,HIGH]"
    echo " [OBJECTIVE_1] 	[value of OBJECTIVE_1, given the OBJECTIVE_2 has the value on previous line]"
    echo " Status   	[status code from the CPLEX run for constrained OBJECTIVE_1]"
    echo " Runtime	[runtime of the CPLEX run for constrained OBJECTIVE_1]"
    exit 1
fi

# @return a string version of a value, by substituting a 'p' for a decimal point
# if necessary
string_version() {
    # replace decimal point with a 'p'
    local objective=$1
    local objective_string=${objective/\./p}
    echo $objective_string
}

# runs cplex on base.sgf with the objective
# conditioned on the value of a second objective
# sets the variable min_objective to the [optimal] value if CPLEX run does not crash
# sets the variable status to "Optimal" if CPLEX found the optimal solution
# sets the variable runtime to the runtime of the CPLEX run (uses CPLEX internal clock)
# Usage: run_cplex SGF_FILE OBJECTIVE CONDITIONAL_OBJECTIVE CONDITIONAL_VALUE
run_cplex() {
    local sgf_file=$1
    local base=${input_sgf_file%.sgf}
    shift
    local objective=$1
    shift
    local conditional_objective=$1
    local conditional_value=$2
    local suffix=${conditional_objective}_${conditional_value}_${objective}
    local cplex_input_file=${base}-$suffix.lp
    local cplex_output_file=${base}-$suffix.out
    local cplex_creation_flags="--objective=$objective --$conditional_objective=$conditional_value"

    errcho "_____________"
    if [ -e $cplex_input_file ]; then
        errcho "__ $cplex_input_file already exists, not creating"
    else
        errcho "__ creating ILP, objective is $objective, input file is $input_sgf_file"
        errcho " __ conditional_objective is $conditional_objective, value = $conditional_value"
        $script_dir/sgf2ilp.py $cplex_creation_flags < $sgf_file > $cplex_input_file
    fi
    if [ -e $cplex_output_file ]; then
        errcho "__ already run CPLEX on $cplex_input_file, not repeating"
    else
        errcho -n "__ solving $cplex_input_file,      "
        date -u 1>&2
        cplex_ilp $CPLEX_OPTIONS -solution $cplex_input_file > $cplex_output_file
        errcho -n "__ done solving $cplex_input_file, "
        date -u 1>&2
    fi
    min_objective=`grep '^value' $cplex_output_file | cut -f 2`
    status=`grep '^StatusCode' $cplex_output_file | cut -f 2`
    runtime=`grep '^runtime' $cplex_output_file | cut -f 2`
    CPXtime=`grep '^CPXtime' $cplex_output_file | cut -f 2`
    errcho -n "__ minimum value for $objective"
    errcho -n " given $conditional_objective=$conditional_value is $min_objective"
    errcho ", status is $status"
    errcho "----------------"
}

# python scripts are in same directory as this one
script_dir=${0%/*}
input_sgf_file=$1
instance_name=`basename $input_sgf_file .sgf`
shift
objective_1=$1
shift
objective_2=$1
shift
low=$1
shift
high=$1
shift
cplex_time=CPLEX_TIME
if [ $# -eq 1 ]; then
    cplex_time=$1
fi
CPLEX_OPTIONS="$CPLEX_OPTIONS -time=$cplex_time"

errcho -n "__ sgf_file = $input_sgf_file, objective_1 = $objective_1, objective_2 = $objective_2"
errcho ", low = $low, high = $high"

# minimize objective_1 for each value of objective_2 in range [low,high]
value=$low
while [ $value -le $high ]; do
    # minimize objective_1 given objective_2 = value
    run_cplex $input_sgf_file $objective_1 $objective_2 $value
    echo "00_Instance $instance_name"
    echo "$objective_2 $value"
    echo "$objective_1 $min_objective"
    echo "Status $status"
    echo "Runtime $runtime"
    echo "CPXtime $CPXtime"
    value=$(( value + 1 ))
done
