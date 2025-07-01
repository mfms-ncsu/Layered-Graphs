#! /usr/bin/env bash

# runs a sequence of heuristics, using minimization software, ../src/minimization,
# a specified number of times, feeding optimum output of each run to the input of the next;
# the sequence of heuristics and their options is read from a config file;
# an optional argument gives a seed, allowing randomized versions of the heuristics;
# an example of a config file is test_repeatHeuristics.txt;

# echo to stderr
errcho() { >&2 echo "$@"; }

usage() {
    errcho "Usage: repeatHeuristics.sh INPUT.sgf CONFIG_FILE REPETITIONS [SEED]"
    errcho "   CONFIG_FILE is a sequence of lines giving options for the minimization program"
    errcho "   REPETITIONS is an integer specifying the number of times the sequence is to be repeated"
    errcho "      the first heuristic is not repeated"
    errcho "   if SEED is specified, heuristics are randomized, using SEED as an initial seed"
    errcho "      (the seed is incremented each time minimization is run again)"
}

if [ $# -lt 3 ] || [ $# -gt 4 ]; then
    usage
    exit 1
fi

# if the script is in script_dir, then the executable is in script_dir/../src
script_dir=${0%/*}
executable=$script_dir/../src/minimization

input_file=$1
if [ ! -f $input_file ]; then
    errcho "Input file $input_file does not exist"
    usage
    exit 1
fi

shift
config_file=$1
if [ ! -f $config_file ]; then
    errcho "Config file $config_file does not exist"
    usage
    exit 1
fi

shift
repetitions=$1
shift
if [ $# -eq 1 ]; then
    seed=$1
fi

# executes minimization with the options specified by first arg; second arg, if present, is seed
# reads from standard input and outputs to standard output using the -I and -O options
exec() {
    options="$1"
    if [ $# -eq 2 ]; then
        random_opt="-R $2"
    else
        random_opt=""
    fi
    options="$options -I -O $random_opt"
    $executable $options
    status=$?
    if [ $status -ne 0 ]; then
        errcho "*** fatal error: $executable $options failed ***"
        exit $status
    fi
}

# read congif file into an array
declare -a heuristics
while read line; do
    heuristics+=("$line")
done < $config_file

output_file=/tmp/`basename $input_file .sgf`-out_$$.sgf
intermediate_file=/tmp/`basename $input_file .sgf`-in_$$.sgf

# repeat the sequence of heuristics a number of times specified by repetitions
# the first heuristic is run only once
errcho
errcho "+++ input_file is $input_file"
options="${heuristics[0]}"
errcho "  +++ running with $input_file using $options"
echo "  +++ running with $input_file using $options"
exec "$options" $seed < $input_file > $intermediate_file
sequence_number=1
while [ $((repetitions)) -gt 0 ]; do
    for options in "${heuristics[@]:1}"; do
        errcho "  +++ running with $input_file using $options"
        echo "  +++ running with $input_file using $options"
        cp $intermediate_file /tmp/input-$sequence_number.sgf
        exec "$options" $seed < $intermediate_file > $output_file
        cp $output_file $intermediate_file
        if [ -n "$seed" ]; then
            let seed+=1
        fi
        let sequence_number+=1
    done
    let repetitions-=1
done

# run with no heuristic to get usual output
$executable -I < $intermediate_file
