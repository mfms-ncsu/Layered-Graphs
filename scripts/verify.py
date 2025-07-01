#! /usr/bin/env python3

"""
verify.py - checks the value of various objective functions
"""

import argparse
import sys

def parse_arguments():
    parser = argparse.ArgumentParser(formatter_class=argparse.RawTextHelpFormatter,
                            description='Takes an sgf file and prints values of objective functions:\n'
                          )
    parser.add_argument('input_file', type=str,
                        help='an sgf file')
    args = parser.parse_args()
    return args

####### - global data structures - ########

"""
@todo use classes with data fields for nodes and edges
"""

"""
name of the graph - if a 't' line exists in the input use that, otherwise the base name of the file
@todo issue a warning if these differ
"""

"""
_node_dictionary[node_id] is a dictionary of node dictionary,
          where each node dictionary has keys layer, position, and index
"""
_node_dictionary = {}
"""
_nodes_on_layer[i] is a list of (ids of) nodes on layer i
"""
_nodes_on_layer = []
"""
_edge_dictionary is a dictionary of edge dictionaries,
             where each edge dictionary has keys up_node, down_node, and crossings
"""
_edge_dictionary = {}
"""
_edges_in_channel[i] is a list of edges with endpoints on layers i and i + 1
"""
_edges_in_channel = []

"""
The relevant statistics
"""
_crossings = 0
_bottleneck_crossings = 0
_nonverticality = 0
_bottleneck_verticality = 0
_stretch = 0
_bottleneck_stretch = 0
 
def graph_name():
    return _name

def number_of_nodes():
    return sum([ len(_nodes_on_layer[k]) for k in range(len(_nodes_on_layer)) ])

def number_of_edges():
    return sum([ len(_edges_in_channel[k]) for k in range(len(_nodes_on_layer)) ])

def number_of_layers():
    return len(_nodes_on_layer)

def read_sgf(input):
    global _node_dictionary
    global _nodes_on_layer
    global _edges_in_channel
    global _name
    line = skip_comments(input)
    # for now, assume next line begins with 't'; do error checking later
    # since the number of nodes and edges is implicit, these can be ignored
    _name = line.split()[1]
    line = read_nonblank(input)
    while (line):
        type = line.split()[0]
        if type == 'n':
            process_node(line)
        elif type == 'e':
            process_edge(line)
            # otherwise error (ignore for now)
        line = read_nonblank( input )
    _nodes_on_layer = layer_lists()
    sort_layers_by_position()
    _edges_in_channel = channel_lists()

"""
# reads and skips lines that begin with 'c'
# @return the first line that is not a comment line
"""
def skip_comments( input ):
    line = read_nonblank( input )
    while ( line.split()[0] == 'c' ):
        line = read_nonblank( input )
    return line

"""
@return the next non-blank line in the input
"""
def read_nonblank(input):
    line = input.readline()
    while ( line and line.strip() == "" ):
        line = input.readline()
    return line

"""
creates a dictionary entry corresponding to the node information on the line
"""
def process_node(line):
    global _node_dictionary
    line_fields = line.split()
    id = line_fields[1]
    layer = int(line_fields[2])
    position = int(line_fields[3])
    _node_dictionary[id] = {'layer': layer, 'position': position}

def process_edge(line):
    global _edge_dictionary
    line_fields = line.split()
    source = line_fields[1]
    target = line_fields[2]
    _edge_dictionary[(source, target)] = {}
    _edge_dictionary[(source, target)]['down_node'] = source
    _edge_dictionary[(source, target)]['up_node'] = target
    _edge_dictionary[(source, target)]['crossings'] = 0
    _edge_dictionary[(source, target)]['nonverticality'] = 0

"""
@return a list whose i-th entry is a list of nodes on layer i
"""
def layer_lists():
    # first compute the maximum layer number
    max_layer = 0
    for node_id in _node_dictionary:
        layer = _node_dictionary[node_id]['layer']
        if layer > max_layer:
            max_layer = layer
    # since layers are numbered starting with 0, the number of layers is
    # actually max_layer + 1; also, need to be careful that there are
    # multiple copies of the empty list in 'layers'
    layers = []
    for layer in range(max_layer + 1):
        layers.append([])
    for node_id in _node_dictionary:
        layer = _node_dictionary[node_id]['layer']
        layers[layer].append(node_id)
    return layers

"""
sort the layer by position and set indexes to be consistent with the sorted order
@param layer_number the index of the layer in _nodes_on_layer
"""
def sort_layer_by_position(layer_number):
    nodes = _nodes_on_layer[layer_number]
    nodes.sort(key = lambda node_id: _node_dictionary[node_id]['position'])
    for index in range(len(nodes)):
        _node_dictionary[nodes[index]]['index'] = index

"""
sort each layer by increasing position and set the index of each node to be its index in the sorted list
"""
def sort_layers_by_position():
    for layer_number in range(len(_nodes_on_layer)):
        sort_layer_by_position(layer_number)

"""
@return a list whose i-th entry is a list of edges with endpoints in layers i and i + 1
Side effect: ensure that all edges (x,y) have x on layer i and y on layer i + 1,
             i.e., are directed toward the larger layer 
"""
def channel_lists():
    # first compute the maximum channel number
    max_channel = 0
    for edge in _edge_dictionary:
        # get the node id's for the two endpoints
        down_node = _edge_dictionary[edge]['down_node']
        up_node = _edge_dictionary[edge]['up_node']
        # get the corresponding layers
        down_node_layer = _node_dictionary[down_node]['layer']
        up_node_layer = _node_dictionary[up_node]['layer']
        channel = down_node_layer
        if down_node_layer > up_node_layer:
            sys.stderr.write("** Warning: edge {} is in the wrong direction **".format(edge))
            _edge_dictionary[edge]['down_node'] = up_node
            _edge_dictionary[edge]['up_node'] = down_node
            channel = up_node_layer
        if channel > max_channel:
            max_channel = channel
    channels = []
    # then put each edge into its channel
    # if channel numbers are 0 through max_channel, ...
    for channel in range(max_channel + 1):
        channels.append([])
    for edge in _edge_dictionary:
        down_node = _edge_dictionary[edge]['down_node']
        down_node_layer = _node_dictionary[down_node]['layer']
        channel = down_node_layer
        channels[channel].append(edge)
    return channels

# -------- here ends the reading of the graph --------------- #

"""
Two edges (w,y) and (x,z) have a crossing if one of the following holds
    - w is to the left of x and z is to the *right* of y
    - w is to the right of x and z is to the *left* of y
assuming w and x are on the same layer as are y and z
"""
def update_crossings(edge_one, edge_two):
    global _crossings
    global _bottleneck_crossings
    edge_one_dictionary = _edge_dictionary[edge_one]
    edge_two_dictionary = _edge_dictionary[edge_two]
    w = edge_one_dictionary['down_node']
    x = edge_two_dictionary['down_node']
    y = edge_one_dictionary['up_node']
    z = edge_two_dictionary['up_node']
    w_position = _node_dictionary[w]['position']
    x_position = _node_dictionary[x]['position']
    y_position = _node_dictionary[y]['position']
    z_position = _node_dictionary[z]['position']
    if ( w_position < x_position and y_position > z_position ) \
        or ( w_position > x_position and y_position < z_position ):
        _crossings += 1
        edge_one_dictionary['crossings'] += 1
        edge_two_dictionary['crossings'] += 1
    if edge_one_dictionary['crossings'] > _bottleneck_crossings:
        _bottleneck_crossings = edge_one_dictionary['crossings']
    if edge_two_dictionary['crossings'] > _bottleneck_crossings:
        _bottleneck_crossings = edge_two_dictionary['crossings']

"""
compute both number of crossings and bottleneck crossings using a brute force method:
in each channel, compare every edge to every other edge to see if they cross or not
"""
def compute_crossings():
    for channel_number in range(len(_edges_in_channel)):
        channel_edges = _edges_in_channel[channel_number]
        size = len(channel_edges)
        for i in range(size):
            for j in range(i + 1, size):
                update_crossings(channel_edges[i], channel_edges[j])

def compute_verticality():
    global _nonverticality
    global _bottleneck_verticality
    for edge in _edge_dictionary:
        down_node_id = _edge_dictionary[edge]['down_node']
        up_node_id = _edge_dictionary[edge]['up_node']
        down_node_dictionary = _node_dictionary[down_node_id]
        up_node_dictionary = _node_dictionary[up_node_id]
        down_position = down_node_dictionary['position']
        up_position = up_node_dictionary['position']
        diff = up_position - down_position
        edge_nonverticality = diff * diff
        _nonverticality += edge_nonverticality
        if edge_nonverticality > _bottleneck_verticality:
            _bottleneck_verticality = edge_nonverticality

"""
@return the stretch of an edge between the two nodes based on the size of their layers
@param node_one_dictionary the dictionary for one of the nodes, for easy retrieval of relevant info
@param node_two_dictionary the dictionary for the other node, for easy retrieval of relevant info
"""
def compute_edge_stretch(node_one_dictionary, node_two_dictionary):
        # @todo consider handling the case of a degree 1 node that is the only node on its layer differently
        #       this would require keeping track of up and down degrees
        # The relevant cases are [neither is consistent with current algorithms]
        #   - degree one on an outer layer with width 1 => stretch = 0, line the node up with it's neighbor
        #   - both up and down degrees are 1 => base stretch on positions of neighbors,
        #     i.e., put the node in a straight line between its neighbors
        #           [this is trickier and not consistent with algorithms; they center a single node on a layer]
        first_index = node_one_dictionary['index']
        second_index = node_two_dictionary['index']
        first_layer = node_one_dictionary['layer']
        second_layer = node_two_dictionary['layer']
        # the following is the interpretation used by algorithms:
        # nodes are evenly spaced with first and last nodes on a layer at the boundaries of a unit rectangle
        first_layer_width = len(_nodes_on_layer[first_layer])
        second_layer_width = len(_nodes_on_layer[second_layer])
        if first_layer_width == 1:
            first_adjusted_position = 0.5 * first_layer_width
        else:
            first_gap = 1.0 / (first_layer_width - 1)
            first_adjusted_position = first_index * first_gap

        if second_layer_width == 1:
            second_adjusted_position = 0.5 * second_layer_width
        else:
            second_gap = 1.0 / (second_layer_width - 1)
            second_adjusted_position = second_index * second_gap
        edge_stretch = abs(first_adjusted_position - second_adjusted_position)
        return edge_stretch

"""
The stretch of an edge is based on a layout that spaces nodes on each layer evenly;
    the outermost nodes on the layer are at positions 0 and 1, others are evenly spaced;
    if there is only one node a layer, it is centered at position 1/2
"""
def compute_stretch():
    global _stretch
    global _bottleneck_stretch
    for edge in _edge_dictionary:
        down_node_id = _edge_dictionary[edge]['down_node']
        up_node_id = _edge_dictionary[edge]['up_node']
        down_node_dictionary = _node_dictionary[down_node_id]
        up_node_dictionary = _node_dictionary[up_node_id]
        edge_stretch = compute_edge_stretch(down_node_dictionary, up_node_dictionary)
        _stretch += edge_stretch
        if edge_stretch > _bottleneck_stretch:
            _bottleneck_stretch = edge_stretch

def compute_objectives():
    compute_crossings()
    compute_verticality()
    compute_stretch()

def print_objectives():
    print("Crossings,{}".format(_crossings))
    print("BottleneckCrossings,{}".format(_bottleneck_crossings))
    print("Nonverticality,{}".format(_nonverticality))
    print("BottleneckVerticality,{}".format(_bottleneck_verticality))
    print("Stretch,{}".format(_stretch))
    print("BottleneckStretch,{}".format(_bottleneck_stretch))

if __name__ == '__main__':
    args = parse_arguments()
    input_stream = open(args.input_file, "r")
    read_sgf(input_stream)
    compute_objectives()
    print_objectives()
