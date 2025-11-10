#! /bin/bash

# generic script for running a program on a set of instances in the same directory
# [from mfms/Utilities repository]

# beginning of line before a record; this will be followed by a description and a date
START_TAG="======"
# line after a record
END_TAG="======"

errcho() { >&2 echo "$@"; }

usage() {
    errcho "Usage: $0 EXECUTABLE INSTANCE_DIR OUTPUT_DIR [SUFFIX] [+] [OPTIONS]"
    errcho "  runs the executable on every instance in INSTANCE_DIR,"
    errcho "  produces an output file with name BASE-SUFFIX in the OUTPUT_DIR"
    errcho "  where BASE is the base name of INSTANCE_DIR"
    errcho "  OPTIONS is a list of options for the EXECUTABLE"
    errcho "  if the + is present, the options come after the input file"
    errcho "  the script can decide whether or not there is a suffix:"
    errcho "     - if the argument after the OUTPUT_DIR begins with '-' there is no suffix"
    errcho "     - otherwise the list of options follows the SUFFIX"
    errcho "  Options usually begin with '-' but they could also be plain arguments that either precede or follow the file name"
    errcho "  However, if they follow the file name, *there must be a SUFFIX*; otherwise + becomes the suffix."
}


if [ $# -lt 3 ]; then
    usage $0
    exit 1
fi

# avoid core dumps
ulimit -c 0

executable=$1
shift
class_dir=$1
shift
output_dir=$1
shift
suffix=$1
first_symbol=${suffix:0:1}
if [ -n "$suffix" ] && [ "$first_symbol" != "-" ]; then
    suffix="-$suffix"
    shift
else
    suffix=
fi
if  [ $1 = "+" ]; then
    options_after="yes";
    shift
fi

options="$@"
if [ ! -d $output_dir ]; then
    if [ ! -f $output_dir ]; then
        mkdir $output_dir
    else
        errcho "Output directory $output_dir exists as a file."
        errcho "Cannot proceed with script $0"
        exit 1
    fi
fi

class_base=`basename $class_dir`
output_file=$output_dir/${class_base}$suffix.out
err_file=$output_dir/${class_base}$suffix.err
if [ -e $output_file ]; then
    output_file=$output_dir/`basename $output_file .out`_$$.out
    errcho "Warning: output file exists, using $output_file"
fi

ls -l `which $executable`
errcho "*** Is $executable up to date? Stop if not! ***"

echo -n "*** " >> $output_file
ls -l `which $executable` >> $output_file
echo "HOSTNAME = $HOSTNAME" >> $output_file
echo "HOSTTYPE = $HOSTTYPE" >> $output_file
echo "MACHTYPE = $MACHTYPE" >> $output_file
echo "OSTYPE = $OSTYPE" >> $output_file

# put system info at the top of the output file
if [[ "$OSTYPE" == linux* ]]; then
    cat /proc/cpuinfo >> $output_file
    cat /proc/meminfo >> $output_file
elif [[ "$OSTYPE" == darwin* ]]; then
    /usr/sbin/system_profiler -detailLevel -2 >> $output_file
elif [[ "$OSTYPE" == solaris* ]]; then
    /usr/platform/sun4u/sbin/prtdiag >> $output_file
fi
echo "" >> $output_file

num_errors=0
for file in $class_dir/*; do
    command_line="$executable $options $file"
    if [ -n "$options_after" ]; then
        command_line="$executable $file $options"
    fi
    echo -n "running $command_line -- "
    date
    echo -n "$START_TAG $command_line, " >> $output_file
    errcho -n "$START_TAG $command_line, " 2>> $err_file
    date -u >> $output_file
    errcho `date -u` 2>> $err_file
    $command_line >> $output_file 2>> $err_file
    if [ $? -ne 0 ]; then
        ((num_errors++))
    fi
    echo "$END_TAG" >> $output_file
    errcho "$END_TAG" 2>> $err_file
    echo "" >> $output_file
    echo -n "--------- "
    date
done
if [ $num_errors -eq 0 ]; then
    echo "Run was successful, no errors encountered, output is in $output_file"
    echo "Output to stderr, if desired, is in $err_file"
else
    echo "*** There were $num_errors errors during the run, see $err_file for details ***"
fi
