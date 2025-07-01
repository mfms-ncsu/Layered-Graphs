#! /bin/bash
# creates a set of permuted instances of a given graph instance in sgf format
# uses scrambleSgf.py

if [ $# != 4 ]; then
    echo "Usage $0 INSTANCE.sgf OUTPUT_DIR NUM_INSTANCES SEED"
    echo " creates a directory containing scrambled versions of INSTANCE.sgf"
    echo " instances are named INSTANCE-0001.sgf to INSTANCE-nnnn.sgf,"
    echo "   where nnnn is a four digit version of NUM_INSTANCES"
    echo " the original instance becomes INSTANCE-0000.sgf"
    echo "   so total number is NUM_INSTANCES + 1"
    exit 1
fi

script_directory=${0%/*}

instance=$1
shift
output_dir=$1
shift
num_instances=$1
shift
seed=$1
base=`basename $instance .sgf`

scramble_script=$script_directory/scrambleSgf.py

if [ ! -d $output_dir ]; then
    if [ ! -f $output_dir ]; then
        mkdir $output_dir
    else
        echo "Output directory $output_dir exists as a file."
        echo "Cannot proceed with script $0"
        exit 1
    fi
fi

cp $instance $output_dir/$base-0000.sgf
seq_number=1
while [[ $seq_number -le $num_instances ]]; do
    seq_string=`echo $seq_number | awk '{printf "%04d",$1}'`
    $scramble_script $instance $seed > $output_dir/$base-$seq_string.sgf
    echo "created instance $output_dir/$base-$seq_string.sgf"
    let seed+=1
    let seq_number+=1
done
