#! /usr/bin/python

"""
 Creates a .dot and .ord file for a layered dag
 representing a lattice on a set of a specified number of elements.
 Nodes are numbered so that the binary representation of each node number is
 a bit vector for the corresponding subset.

@todo make this work for sgf files
"""

import sys

# @return the number of 1's in the binary representation of n
def num_ones( n ):
    ones = 0
    while n > 0:
        if n % 2 == 1:
            ones = ones + 1
        n = n >> 1
    return ones
# end of num_ones

# @return a list of numbers that are
#   - greater than n
#   - differ from n by exactly one bit
#   [this implies that n has a 0 where the neighbor has a 1]
#   - have at most num_bits bits in their binary representation
def neighbors( n, num_bits ):
    adjacent_items = []
    current_n = n
    bit_to_flip = 0
    while bit_to_flip < num_bits:
        if current_n % 2 == 0:
            new_neighbor = n ^ ( 1 << bit_to_flip )
            adjacent_items.append( new_neighbor )
        current_n = current_n >> 1
        bit_to_flip = bit_to_flip + 1
    return adjacent_items
# end of neighbors

def main():
    size = int( sys.argv[1] )
    base_name = "lattice_%02d" % size
    dot_file = open( base_name + ".dot", "w" )
    ord_file = open( base_name + ".ord", "w" )

    # create an empty list for each layer
    layers = [[]] * (size + 1)

    # make sure all the lists are distinct using "slicing" to get deep copies
    for i in range(0, size + 1):
        layers[i] = layers[i][:]
    
    # nodes are numbered from 0 to 2^size - 1
    node_list = list(range( 0, 1 << size))

    # each set is put on a layer corresponding to its number of elements
    for node in node_list:
        layers[ num_ones( node ) ].append( node )

    # write the ord file one layer at a time
    ord_file.write( "# Canonical ordering for lattice_%02d:\n" % size )
    ord_file.write( "# a lattice on a set with %d elements\n" % size )
    NODES_PER_LINE = 10
    nodes_written = 0
    for i in range(0, size + 1):
        ord_file.write( "\n# Order for layer %d\n" % i )
        ord_file.write( "%d {\n" % i )
        for node in layers[i]:
            if nodes_written > NODES_PER_LINE:
                ord_file.write( "\n" )
                nodes_written = 0
            ord_file.write( " n_%o" % node )
        ord_file.write( "\n} # end of layer %d\n" % i )

    # write the dot file, considering the neighbors of each node in turn
    dot_file.write( "/* lattice on a set of %d elements */\n" % size )
    dot_file.write( "digraph lattice_%02d {\n" % size )
    for node in node_list:
        for neighbor in neighbors( node, size ):
            dot_file.write( " n_%o -> n_%o;\n" % ( node, neighbor ) )
    dot_file.write( "}\n" )

    dot_file.close()
    ord_file.close()
# end of main

main()

#  [Last modified: 2020 05 18 at 15:11:26 GMT]
