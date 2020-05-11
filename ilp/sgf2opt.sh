#! /bin/bash

# sgf2opt - starts with an sgf file and finds orderings that minimize two
# distinct objectives; three different optimizations are done: first
# objective alone, second objective alone, and second given the optimum value
# of first as a constraint.

# assumes sgf2ilp.py and sol2sgf.py are in this directory

CPLEX_OPTIONS="-time=3600 -feasible=4 -vsel=n -trace=2"

if [ $# -ne 2 ]; then
    echo "Usage: $0 ( -tb | -ts | -bt | -bs | -st | -sb | -tv | -vt ) FILE.sgf"
    echo " produces files of the form, in the same directory as the input"
    echo "  FILE-TAG.lp (ILP for the appropriate problem)"
    echo "  FILE-TAG.out (the cplex output when the ILP is solved)"
    echo "  FILE-TAG.sgf, (the sgf file with the optimum order)"
    echo " -xy means minimize x then y and then y given the minimum x"
    echo " TAG is one of t, b, s, tk_b, tk_s, bk_t, bk_s, sk_t, sk_b, tk_v, vk_t"
    echo "  x (t,b,s,v) means x is minimized with no constraints"
    echo "  xk_y means y is minimized given that the x has (minimum) value k"
    echo " t = total crossings"
    echo " b = bottleneck crossings"
    echo " s = stretch"
    echo " v = total (non)verticality (quadratic measure)"
    exit 1
fi

# returns a (string representing a) floating point number that is slightly
# rounded up from the input (also a string)
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
    local rounded_number=`echo $min_first_objective + $to_be_added | bc`
    echo $rounded_number
}

# returns the objective function name corresponding to its one letter symbol
symbol_to_objective() {
    local symbol=$1
    local objective=""
    if [ "$symbol" = "t" ]; then
        objective=total
    elif [ "$symbol" = "b" ]; then
        objective=bottleneck
    elif [ "$symbol" = "s" ]; then
        objective=stretch
    elif [ "$symbol" = "v" ]; then
        objective=vertical
    else
        echo "symbol_to_objective: bad objective symbol $symbol"
        exit
    fi
    echo $objective
}

# python scripts are in same directory as this one
script_dir=${0%/*}

objective_arg=$1
echo "objective_arg = $objective_arg"
# peel off the -
objective=${objective_arg#-}
first_objective_symbol=${objective:0:1}
first_objective=$(symbol_to_objective $first_objective_symbol)
second_objective_symbol=${objective:1:1}
second_objective=$(symbol_to_objective $second_objective_symbol)
shift
input_sgf_file=$1
input_base=${input_sgf_file%.sgf}

# minimize the first objective
cplex_input_file=${input_base}-$first_objective_symbol.lp
cplex_output_file=${input_base}-$first_objective_symbol.out
sgf_output_file=${input_base}-$first_objective_symbol.sgf
echo "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%"
if [ -e $cplex_input_file ]; then
    echo "__ $cplex_input_file already exists, not creating"
else
    echo "creating ILP, objective is $first_objective, input file is $input_sgf_file"
    $script_dir/sgf2ilp.py --objective=$first_objective < $input_sgf_file > $cplex_input_file
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

min_first_objective=`fgrep Objective $cplex_output_file | cut -f 2`
echo "minimum value for $first_objective is $min_first_objective"
if [ -n "$min_first_objective" ]; then
    echo "converting solution to sgf for optimal order"
    $script_dir/sol2sgf.py < $cplex_output_file > $sgf_output_file
    if [[ $min_first_objective != ${min_first_objective%.[0-9]*} ]]; then
        # floating point number - need to round up just in case
        min_first_objective=$(round_up $min_first_objective)
        min_first_objective_string=${min_first_objective/\./p}
    else
        min_first_objective_string=$min_first_objective
    fi
else
    echo "!!! no solution found for $cplex_input_file !!!"
fi

# minimize second objective given minimum first objective as a constraint
suffix=${first_objective_symbol}${min_first_objective_string}_$second_objective_symbol
cplex_input_file=${input_base}-$suffix.lp
cplex_output_file=${input_base}-$suffix.out
sgf_output_file=${input_base}-$suffix.sgf
if [ -e $cplex_input_file ]; then
    echo "__ $cplex_input_file already exists, not creating"
else
    bound="--$first_objective=$min_first_objective"
    objective=$second_objective
    echo "creating ILP, objective is $objective, using $first_objective = $min_first_objective"
    $script_dir/sgf2ilp.py --objective=$objective $bound < $input_sgf_file > $cplex_input_file
fi
if [ -e $cplex_output_file ]; then
    echo "__ already run CPLEX on $cplex_input_file, not repeating"
else
    echo -n "solving $cplex_input_file,      "
    date -u
    cplex_ilp $CPLEX_OPTIONS -solution $cplex_input_file > $cplex_output_file
    echo -n "done solving $cplex_input_file, "
    date -u
    echo "converting solution to sgf for optimal order"
    $script_dir/sol2sgf.py < $cplex_output_file > $sgf_output_file
fi

###########################################################

# get the minimum value for the conditional second objective -- for
# comparison purposes
conditional_min_second_objective=`fgrep Objective $cplex_output_file | cut -f 2`
echo "minimum value for $second_objective given $first_objective = $rounded_min_first_objective is $conditional_min_second_objective"

#############################################

# minimize the second objective
cplex_input_file=${input_base}-$second_objective_symbol.lp
cplex_output_file=${input_base}-$second_objective_symbol.out
sgf_output_file=${input_base}-$second_objective_symbol.sgf
if [ -e $cplex_input_file ]; then
    echo "__ $cplex_input_file already exists, not creating"
else
    echo "creating ILP, objective is $second_objective, input file is $input_sgf_file"
    if [ "$second_objective" = "t" ]; then
        $script_dir/sgf2ilp.py --objective=total < $input_sgf_file > $cplex_input_file
    elif [ "$second_objective" = "b" ]; then
        $script_dir/sgf2ilp.py --objective=bottleneck < $input_sgf_file > $cplex_input_file
    elif [ "$second_objective" = "s" ]; then
        $script_dir/sgf2ilp.py --objective=stretch < $input_sgf_file > $cplex_input_file
    elif [ "$second_objective" = "v" ]; then
        $script_dir/sgf2ilp.py --objective=vertical < $input_sgf_file > $cplex_input_file
    else
        echo "bad second objective: $second_objective"
        exit
    fi
fi
if [ -e $cplex_output_file ]; then
    echo "__ already run CPLEX on $cplex_input_file, not repeating"
else
    echo -n "solving $cplex_input_file,      "
    date -u
    cplex_ilp $CPLEX_OPTIONS -solution $cplex_input_file > $cplex_output_file
    echo -n "done solving $cplex_input_file, "
    date -u
    echo "converting solution to sgf for optimal order"
    $script_dir/sol2sgf.py < $cplex_output_file > $sgf_output_file
fi
# get the minimum value for the second objective (for comparison purposes)
min_second_objective=`fgrep Objective $cplex_output_file | cut -f 2`
echo "minimum value for $second_objective is $min_second_objective"
if [[ $min_second_objective != ${min_second_objective%.[0-9]*} ]]; then
    # floating point number - need to round up just in case
    # but only if there are at least six digits (fix this later)
    # @todo may not need to round
    after_point=${min_second_objective##*\.} # digits after decimal point
    number_after_point=${#after_point}
    # create a string of number_after_point - 1 0's followed by a 1 and turn
    # it into a decimal number to be added
    number_of_zeros=$(( $number_after_point - 1 ))
    zeros=`head -c $number_of_zeros < /dev/zero | tr '\0' '0'`
    to_be_added=0.${zeros}1
    rounded_min_second_objective=`echo $min_second_objective + $to_be_added | bc`
    min_second_objective_string=${rounded_min_second_objective/\./p}
else
    rounded_min_second_objective=$min_second_objective
    min_second_objective_string=$min_second_objective
fi

# compare these as strings because bash does not handle floats
if [ "$min_second_objective" != "$conditional_min_second_objective" ]; then
    echo "*+*+* $input_base: objectives differ: $second_objective is $min_second_objective, $second_objective given $first_objective = $min_first_objective is $conditional_min_second_objective *+*+*"
fi
echo "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%"
echo

#  [Last modified: 2017 11 11 at 01:23:54 GMT]
