#! /usr/bin/env python

"""
@todo under construction
"""

"""
  Takes an sgf file and prints statistics about degrees
  of nodes relative to the upper and lower layers of each channel
"""

from argparse import ArgumentParser
from argparse import RawTextHelpFormatter # to allow newlines in help messages
import math
import os
import statistics

def parse_arguments():
    parser = ArgumentParser(formatter_class=RawTextHelpFormatter,
                            description='Takes an sgf file and prints statistics about degrees and layers\n'
                          )
    parser.add_argument('input_file', type=str,
                        help='an sgf file')
    args = parser.parse_args()
    return args

from pathlib import Path

"""
    Returns the base name (filename with extension) from a file path.
    Works cross-platform and handles edge cases.
"""
def get_base_name(file_path):
    if not isinstance(file_path, (str, Path)) or not str(file_path).strip():
        raise ValueError("file_path must be a non-empty string or Path object")
    
    return Path(file_path).name

"""
creates the global data structures _node_dictionary and _nodes_on_layer,
where
 in _node_dictionary each entry has key = a node id and value = a list
        [layer, up_degree, down_degree]
and _nodes_on_layer[i] is a list of (ids of) nodes on layer i
"""
_node_dictionary = {}
_nodes_on_layer = []
 
def layer(node):
    return _node_dictionary[node][0]

def updegree(node):
    return _node_dictionary[node][1]
 
def downdegree(node):
    return _node_dictionary[node][2]

def graph_name():
    return _name

def number_of_nodes():
    return sum([ len(_nodes_on_layer[k]) for k in range(len(_nodes_on_layer)) ])

def number_of_edges():
    upper_degree_list = [ updegree(x) for x in _node_dictionary ]
    return sum(upper_degree_list)

def number_of_layers():
    return len(_nodes_on_layer)

def read_sgf(input):
    global _node_dictionary
    global _nodes_on_layer
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
    line_fields = line.split()
    id = line_fields[1]
    layer = int(line_fields[2])
    _node_dictionary[id] = [layer, 0, 0]

"""
updates up- and down-degrees for the endpoints of the edge described on the line
"""
def process_edge(line):
    line_fields = line.split()
    source = line_fields[1]
    target = line_fields[2]
    _node_dictionary[source][1] += 1
    _node_dictionary[target][2] += 1

"""
@return a list whose i-th entry is a list of nodes on layer i
"""
def layer_lists():
    # first compute the maximum layer number
    max_layer = 0
    for node in _node_dictionary:
        layer = _node_dictionary[node][0]
        if layer > max_layer:
            max_layer = layer
    # since layers are numbered starting with 0, the number of layers is
    # actually max_layer + 1; also, need to be careful that there are
    # multiple copies of the empty list in 'layers'
    layers = []
    for layer in range(max_layer + 1):
        layers.append([])
    for node in _node_dictionary:
        layer = _node_dictionary[node][0]
        layers[layer].append(node)
    return layers

# -------- here ends the reading of the graph --------------- #

# ----------------------------------------------------------- #

# -------- some possibly useful utilitities ----------------- #

"""
@return the weighted degree of the node x, defined as
     deg(x) * |x's layer| / |opposite layer| if |opposite_layer| > |x's layer|
     deg(x) otherwise
  idea is that a large degree node has less significance if the size of the opposing
  layer is large
@param x the node id
@param channel the channel with respect to which the weighted degree is being calculated
    - if the node is on layer i and channel = i then we care about the upward degree
      and the opposite side is layer i + 1
    - if the node is on layer i and channel = i - 1 we care about the downward degree
      and the opposite side is layer i - 1
"""
def weighted_degree(x, channel):
    this_layer = layer(x)
    this_size = len(_nodes_on_layer[this_layer])
    if channel == this_layer:
        degree = updegree(x)
        opposite_size = len(_nodes_on_layer[this_layer + 1])
    elif channel == this_layer - 1:
        degree = downdegree(x)
        opposite_size = len(_nodes_on_layer[this_layer - 1])
    else:
        print("*** Error in weighted degree: node {} not in channel {}".format(x, channel))
        exit
    if opposite_size <= this_size:
        return degree
    return degree / opposite_size

# ------- print the following information in a format that can by used by runStats2csv ----
# - # nodes
# - # edges
# - # layers
# average # nodes per layer can be calculated from the above
# ditto average channel density: divide # edges by (# layers - 1)
# - median nodes/layer
# - variance nodes/layer
# - median channel density
# - variance channel density
# - average (relative) channel degree of nodes [related to channel density, but depends on layer widths]
# - median (relative) channel degree of nodes
# - variance (relative) channel degree of nodes

"""
adds to the output list a list of pairs, representing min, median, mean, max, and variance
    of numbers in a given list
@param output_list the list to which items are to be added [is modified]
@param number_list the list of numbers that the statistics will be based on
@param tag a string used as a suffix for 'min', 'med', 'avg', 'max', and 'var'
"""
def add_statistics(output_list, number_list, tag):
    output_list.append(("min_"+tag, min(number_list)))
    output_list.append(("med_"+tag, statistics.median(number_list)))
    output_list.append(("avg_"+tag, statistics.mean(number_list)))
    output_list.append(("max_"+tag, max(number_list)))
    if len(number_list) < 2:
        var = 0
    else:
        var = statistics.variance(number_list)
    output_list.append(("var_"+tag, var))

"""
adds statistics related tp number of layers and nodes per layer to the output list
"""
def add_layer_statistics(output_list):
    layer_widths = [ len(x) for x in _nodes_on_layer ]
    add_statistics(output_list, layer_widths, "width")

# -- a channel is defined to be two consecutive layers k and k+1 for k = 0 to #layers - 2
#    nodes of a channel are the nodes on both layers
#    edges are the upward edges of layer k and the downward ones of layer k+1

"""
@return a list of the nodes in the given channel, excluding those whose channel degree is 0
@assume channel >= 0 and < # of layers - 2
"""
def nodes_in_channel(channel):
    bottom_layer = [ x for x in _nodes_on_layer[channel] if updegree(x) > 0 ]
    top_layer = [ x for x in _nodes_on_layer[channel + 1] if downdegree(x) > 0 ]
    return bottom_layer + top_layer

"""
@return the number of nodes in the given channel
@assume channel >= 0 and < # of layers - 2
"""
def num_nodes_in_channel(channel):
    return len(nodes_in_channel(channel))

"""
@return the number of edges in the given channel
@assume channel >= 0 and < # of layers - 2
"""
def num_edges_in_channel(channel):
    upper_degrees = [ updegree(x) for x in _nodes_on_layer[channel] ]
    return sum(upper_degrees)

"""
adds basic information about the graph
"""
def add_basic_information(output_list):
    output_list.append(("filename", get_base_name(_args.input_file)))
    output_list.append(("name", graph_name()))
    output_list.append(("nodes", number_of_nodes()))
    output_list.append(("edges", number_of_edges()))
    output_list.append(("layers", number_of_layers()))

"""
adds statistics related to channel density (# edges / # nodes in channel) to the output list
"""
def add_density_statistics(output_list):
    channel_densities = []
    for channel in range(len(_nodes_on_layer) - 1):
        num_nodes = num_nodes_in_channel(channel)
        num_edges = num_edges_in_channel(channel)
        channel_densities.append(num_edges / num_nodes)
    add_statistics(output_list, channel_densities, "chdens")

"""
adds statistics related to relative channel degrees of nodes,
    where relative channel degree of a node x in channel k = e_k(x) / n'
    here e_k(x) = # of edges in the channel incident on x
    and  n' = # of nodes on the layer opposite x
"""
def add_degree_statistics(output_list):
    degree_list = []
    for channel in range(number_of_layers() - 1):
        channel_degrees = [ weighted_degree(x, channel) for x in nodes_in_channel(channel) ]
        degree_list.extend(channel_degrees)
    add_statistics(output_list, degree_list, "reldeg")

"""
collects all the important statistics and adds them to the output list
"""
def gather_statistics(output_list):
    add_basic_information(output_list)
    add_layer_statistics(output_list)
    add_density_statistics(output_list)
    add_degree_statistics(output_list)

"""
prints a list of tag/value pairs, one per line, with a separator between tag and value
@param output_list a list of pairs of the form (tag, value)
    where tag is a string indicating the type of statistic and value is the corresponding value
@param separator a string used to separate the tag from the value
"""
def print_statistics(output_list, separator="\t"):
    for pair in output_list:
        (tag, value) = pair 
        print("{}{}{}".format(tag, separator, value))

if __name__ == '__main__':
    global _args
    _args = parse_arguments()
    input_stream = open(_args.input_file)
    read_sgf(input_stream)
    output_list = []
    gather_statistics(output_list)
    print_statistics(output_list)
