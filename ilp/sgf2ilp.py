#! /usr/bin/env python3

"""
Converts a graph in .sgf format to a linear integer program
This program translates from standard input to standard output.
"""

import sys
import os

import math
import random
import itertools

from argparse import ArgumentParser
from argparse import RawTextHelpFormatter # to allow newlines in help messages

TOLERANCE = 0
MAX_TERMS_IN_LINE = 100
INDENT = "  "

parser = ArgumentParser(formatter_class=RawTextHelpFormatter,
                                 description='Creates an ILP to minimize an objective based on an sgf representation of a layered graph',
                                 epilog='reads sgf from standard input, prints lp file on standard output')
parser.add_argument('--objective', choices={'total','bottleneck',
                                            'stretch','bn_stretch',
                                            'quadratic', 'vertical'},
                    default='total',
                    required=True,
                    help='minimize ...\n'
                    + ' total/bottleneck (total/bottleneck crossings)\n'
                    + ' stretch/bn_stretch (total/bottleneck edge length)\n'
                    + ' quadratic (use quadratic programming to miminze total stretch)\n'
                    + ' vertical (minimize non-verticality)\n')
parser.add_argument('--total', type=int,
                    help='constraint on the total number of crossings (default: None)')
parser.add_argument('--bottleneck', type=int,
                    help='constraint on the maximum number of crossings of any edge (default: None)')
parser.add_argument('--stretch', type=float,
                    help='constraint on the total edge length (default: None)')
parser.add_argument('--bn_stretch', type=float,
                    help='constraint on the maximum length of any edge (default: None)')
parser.add_argument('--vertical', type=int,
                    help='constraint on the total non-verticality (default: None)')
parser.add_argument('--seed', type=int,
                    help='a random seed if ILP constraints are to be permuted (default: None)')
parser.add_argument('--bipartite', type=int,
                    help='max number of nodes in bipartite constraints for verticality (default: None, 0 means no limit)')

args = parser.parse_args()

# creates three global data structures that describe the graph
#
# _node_dictionary: the item whose key is the id of a node and whose value is
# a tuple of the form (layer, up_neighbors, down_neighbors), where
#     layer is the layer of the node
#     up_neighbors is a list of adjacent nodes on the next higher layer
#     down_neighbors is a list of adjacent nodes on the next lower layer
#
# _edge_list: each item is a tuple of the form:
#       (source, target)
#
# _layer_list: _layer_list[i] is a list of nodes on layer i

_node_dictionary = {}
_edge_list = []
_layer_list = []
_comments = []

def read_sgf(input):
    global _comments
    line = read_nonblank( input )
    while ( line ):
        type = line.split()[0]
        if type == 'n':
            process_node(line)
        elif type == 'e':
            process_edge(line)
        elif type == 'c':
            _comments.append(line[1:])
        line = read_nonblank(input)

# adds a _node_dictionary entry to map a node id to its layer and empty lists
# for the neighbors
def process_node(line):
    global _node_dictionary
    global _layer_list
    line_fields = line.split()
    node_id = line_fields[1]
    layer = int(line_fields[2])
    _node_dictionary[node_id] = (str(layer), [], [])
    number_of_layers = len(_layer_list)
    # make sure layer list has enough entries to accomodate the layer of the
    # new node; there have to be layer+1 at least
    while number_of_layers <= layer:
        _layer_list.append([])
        number_of_layers += 1
    _layer_list[layer].append(node_id)

# adds an edge to the list of edges and adds the endpoints to the appropriate
# lists of neighbors
def process_edge( line ):
    global _edge_list
    global _node_dictionary
    line_fields = line.split()
    source = line_fields[1]
    target = line_fields[2]
    _edge_list.append((source, target))
    # add target to up_neighbors of source and source to the down_neighbors
    # of target
    _node_dictionary[source][1].append(target)
    _node_dictionary[target][2].append(source)

# @return the first non-blank line in the input
def read_nonblank( input ):
    line = input.readline()
    while ( line and line.strip() == "" ):
        line = input.readline()
    return line

# computes the global array _layer_size that gives the size of each layer
#  and the global integer _max_layer_size
# also computes the global array _layer_factor so that _layer_factor[i] gives the
# multiplier on a position variable in layer i when stretch is computed;
# this will be 1/(L-1), where L is the number of nodes in layer i
def compute_layer_factors():
    global _layer_size
    global _max_layer_size
    global _layer_factor
    # first compute number of layers (layer numbers are 0-based) and the size
    # of each
    number_of_layers = len(_layer_list)
    _layer_size = [len(layer) for layer in _layer_list]
    _max_layer_size = max(_layer_size)
    # then the appropriate denominator for each layer
    denominator = [0] * number_of_layers
    for layer in range(number_of_layers):
        this_layer_size = _layer_size[layer]
        if this_layer_size < 1:
            sys.exit("Error: layer " + layer + " has no nodes")
        denominator[layer] = this_layer_size - 1
        # layers with only a single node should put that node in the center
        if denominator[layer] == 0:
            denominator[layer] = 2
    # now we're ready to compute the actual factors
    _layer_factor = [0] * number_of_layers
    for layer in range(number_of_layers):
        _layer_factor[layer] = 1.0 / denominator[layer]

# In what follows a constraint is a tuple of the form
#  (left, relational_operator, right)
# where left is a list of strings,
#       relational_operator is one of '>=', '<=', '=', etc.
#       and right is a string representing a number.
# each string in left is a +, -, a +c or -c (where c is a constant)
# followed by a variable name.

# variables are added to these sets when they arise in constraints
_binary_variables = []
_integer_variables = []
_continuous_variables = []
_crossing_variables = []
_raw_stretch_variables = []
_nonverticality_variables = []

# @return a list of constraints on relative position variables x_i_j, where
# x_i_j is 1 if node i precedes node j on their common layer, 0 otherwise
def triangle_constraints():
    global _binary_variables
    triangle_constraints = []
    # anti-symmetry constraints
    for i in _node_dictionary:
        i_layer = _node_dictionary[i][0]
        for j in _node_dictionary:
            j_layer = _node_dictionary[j][0]
            # add a constraint if i and j on same layer; only once per pair
            if i_layer == j_layer and i < j:
                x_i_j = "x_" + str(i) + "_" + str(j)
                x_j_i = "x_" + str(j) + "_" + str(i)
                current_constraint = (["+ " + x_i_j, "+ " + x_j_i], '=', '1')
                triangle_constraints.append(current_constraint)
                _binary_variables.append(x_i_j)
                _binary_variables.append(x_j_i)

    # transitivity constraints
    # x_i_j and x_j_k implies x_i_k
    # or x_i_k - x_i_j - x_j_k >= -1
    for i in _node_dictionary:
        i_layer = _node_dictionary[i][0]
        for j in _node_dictionary:
            j_layer = _node_dictionary[j][0]
            for k in _node_dictionary:
                k_layer = _node_dictionary[k][0]
                # add two constraints if the three nodes are on the same
                # layer; only once per triple
                if i_layer == j_layer == k_layer and i < j and j < k:
                    relop = '>='
                    right = '-1'

                    left = ["+ x_" + i + "_" + k]
                    left.append("- x_" + i + "_" + j)
                    left.append("- x_" + j + "_" + k)
                    triangle_constraints.append((left, relop, right))

                    left = ["+ x_" + i + "_" + j]
                    left.append("- x_" + i + "_" + k)
                    left.append("- x_" + k + "_" + j)
                    triangle_constraints.append((left, relop, right))

    # the above two variants suffice
    #  x_i_k - x_i_j - x_j_k = (1 - x_k_i) - x_i_j - (1 - x_k_j)
    #                        = x_k_j - x_k_i - x_i_j
    #                        = (1 - x_j_k) - x_k_i - (1 - x_j_i)
    #                        = x_j_i - x_j_k - x_k_i
    return triangle_constraints

# @return a list of position constraints given the node list and edge list
# p_i_el represents the position of node i in layer el
# if node i precedes node j on layer el then p_j_el - p_i_el >= 1, otherwise unconstrained
# can simulate unconstrained using multiplication of x_j_i by _max_layer_size
# if contiguous p_i_el <= layer_size - 1, otherwise it's <= _max_layer_size - 1
def position_constraints(contiguous):
    global _integer_variables
    position_constraints = []
    relop = '>='
    right = '1'
    for node_id in _node_dictionary:
        layer = int(_node_dictionary[node_id][0])
        position_variable = "p_" + node_id + "_" + str(layer)
        _integer_variables.append(position_variable)
        for other_node_id in _node_dictionary:
            other_layer = int(_node_dictionary[other_node_id][0])
            if layer == other_layer and node_id != other_node_id:
                other_position_variable = "p_" + other_node_id + "_" + str(layer)
                left = []
                left.append("+ " + other_position_variable)
                left.append("- " + position_variable)
                left.append("+ " + str(_max_layer_size) + " "
                            + "x_" + other_node_id + "_" + node_id)
                position_constraints.append((left, relop, right))
        if contiguous:
            rightmost_position = _layer_size[layer] - 1
        else:
            rightmost_position = _max_layer_size - 1
        position_constraints.append(([ position_variable ], '<=', rightmost_position))
    return position_constraints

# @return a list of trivial constraints of the form "c_i_j_i_j = 0", one for
# each edge ij, so that each edge is guaranteed to be included when the
# solution is parsed by sol2sgf.py
def edges_for_output():
    global _binary_variables
    edges = []
    for edge in _edge_list:
        source = edge[0]
        target = edge[1]
        edge_variable = "c_" + source + "_" + target + "_"  + source + "_" + target
        edges.append((["+ " + edge_variable], "=", "0"))
        _binary_variables.append(edge_variable)
    return edges

# @return a list of crossing constraints given node list and edge list the
# variable c_i_j_k_l = 1 iff edge ij crosses edge kl, where i,k are on one
# layer and j,l are on the next layer; a crossing occurs if
#      i precedes k and l precedes j
#   or k precedes i and j precedes l
def crossing_constraints():
    global _binary_variables
    global _crossing_variables
    crossing_constraints = []
    relop = '>='
    right = '-1'
    # for every pair of edges edge_1 and edge_2, where the two edges are in
    # the same channel and do not have any common endpoints, generate the two
    # relevant crossing constraints
    for index_1, edge_1 in enumerate(_edge_list):
        source_1 = edge_1[0]
        target_1 = edge_1[1]
        channel_1 = _node_dictionary[target_1][0]
        for index_2, edge_2 in enumerate(_edge_list):
            source_2 = edge_2[0]
            target_2 = edge_2[1]
            channel_2 = _node_dictionary[target_2][0]
            # check if two edges in the same channel without common node
            if channel_1 == channel_2 \
                    and index_1 < index_2 \
                    and source_1 != source_2 and target_1 != target_2:
                crossing_variable = "c_" + source_1 + "_" + target_1 \
                    + "_" + source_2 + "_" + target_2
                _binary_variables.append(crossing_variable)
                _crossing_variables.append(crossing_variable)

                left = ["+ " + crossing_variable]
                # wrong order in the first layer
                left.append("- x_" + str(source_2) + "_" + str(source_1))
                # but correct on second
                left.append("- x_" + str(target_1) + "_" + str(target_2))
                crossing_constraints.append((left, relop, right))

                left = ["+ " + crossing_variable]
                # wrong order in the second layer
                left.append("- x_" + str(target_2) + "_" + str(target_1))
                # but correct on first
                left.append("- x_" + str(source_1) + "_" + str(source_2))
                crossing_constraints.append((left, relop, right))
    return crossing_constraints
    

# @return a list of bottleneck crossing constraints
# a bottleneck constraint has the form
#    bottleneck - sum of all crossing variables for a particular edge >= 0
def bottleneck_constraints():
    global _integer_variables
    _integer_variables.append("bottleneck")
    relop = '>='
    right = '0'
    bottleneck_constraints = []
    for index_1, edge_1 in enumerate(_edge_list):
        source_1 = edge_1[0]
        target_1 = edge_1[1]
        channel_1 = _node_dictionary[target_1][0]
        # compile the left hand side for edges crossing this one
        left = ["+ bottleneck"]
        for index_2, edge_2 in enumerate(_edge_list):
            source_2 = edge_2[0]
            target_2 = edge_2[1]
            channel_2 = _node_dictionary[target_2][0]
            # check if two edges in the same channel without common node
            if channel_1 == channel_2 \
                    and source_1 != source_2 and target_1 != target_2:
                if index_1 < index_2:
                    crossing_variable = "c_" + str(source_1) + "_" + str(target_1) \
                        + "_" + str(source_2) + "_" + str(target_2)
                else:
                    crossing_variable = "c_" + str(source_2) + "_" + str(target_2) \
                        + "_" + str(source_1) + "_" + str(target_1)
                left.append("- " + crossing_variable)
        # add bottleneck constraint if it there's at least one potentially
        # crossing edge in the channel
        if len(left) > 1:
            bottleneck_constraints.append((left, relop, right))

    return bottleneck_constraints
    
# @return a single constraint that captures the fact that the total >= the
# sum of all crossing variables
def total_constraint():
    relop = '>='
    right = '0'
    left = ["+ total"]
    for crossing_variable in _crossing_variables:
        left.append("- " + crossing_variable)
    return (left, relop, right)
    
# @return a list of stretch constraints, each constraint says, essentially,
# that the s_i_j = abs((1/|V_k|) * p_i_k - (1/|V_{k+1}) * p_j_{k+1}), where ij is an
# edge and k is the layer of node i
def stretch_constraints():
    global _continuous_variables
    global _stretch_variables
    _stretch_variables = []
    stretch_constraints = []

    # generate stretch constraints
    relop = '>='
    for edge in _edge_list:
        right = str(-TOLERANCE)
        source = edge[0]
        target = edge[1]
        stretch_variable = "s_" + source + "_" + target
        raw_variable = "z_" + source + "_" + target
        _continuous_variables.append(stretch_variable)
        _stretch_variables.append(stretch_variable)
        # standard tricks for absolute value
        # s_i_j >= z_i_j
        left = ["+ " + stretch_variable]
        left.append("- " + raw_variable)
        stretch_constraints.append((left, relop, right))
        # s_i_j >= -z_i_j
        left = ["+ " + stretch_variable]
        left.append("+ " + raw_variable)
        stretch_constraints.append((left, relop, right))
        # introduce binary indicator variable: b_i_j = 0 if z_i_j is positive
        # and 1 if z_i_j is negative
        indicator_variable = "b_" + source + "_" + target
        _binary_variables.append(indicator_variable)
        # ensure s_i_j <= z_i_j if b_i_j = 0
        left = ["+ " + raw_variable]
        left.append("+ 2 " + indicator_variable)
        left.append("- " + stretch_variable)
        stretch_constraints.append((left, relop, right))
        # ensure s_i_j <= -z_i_j if b_i_j = 1
        left = ["- " + raw_variable]
        left.append("- 2 " + indicator_variable)
        left.append("- " + stretch_variable)
        right = str(-2 - TOLERANCE)
        stretch_constraints.append((left, relop, right))

    return stretch_constraints
    
# @return a list of constraints that give value to variables whose absolute
# values define stretch and whose squares are used in the quadratic
# objective. Each constraint says, essentially, that
# z_i_j = abs((1/|V_k|) * p_i_k - (1/|V_{k+1}) * p_j_{k+1}), where ij is an
# edge and k is the layer of node i; these are also used to get absolute
# value equality constraints for the regular stretch variables
def raw_stretch_constraints():
    global _raw_stretch_variables
    raw_constraints = []

    # generate raw stretch constraints
    relop = '='
    right = '0'
    for edge in _edge_list:
        source = edge[0]
        target = edge[1]
        raw_stretch_variable = "z_" + source + "_" + target
        _raw_stretch_variables.append(raw_stretch_variable)
        source_layer = int(_node_dictionary[source][0])
        target_layer = int(_node_dictionary[target][0])
        source_position_variable = "p_" + source + "_" + str(source_layer)
        target_position_variable = "p_" + target + "_" + str(target_layer)
        left = ["+" + raw_stretch_variable]
        left.append("+ " + str(_layer_factor[source_layer])
                    + " " + source_position_variable)
        left.append("- " + str(_layer_factor[target_layer])
                    + " " + target_position_variable)
        raw_constraints.append((left, relop, right))
    return raw_constraints
    
# @return distance constraints for "linearized" versions of non-verticality
# variables d_u_v_i (see 2011 Jena tech report by Chimani and Hungerlaender);
# add the variables to _integer_variables:
#   d_u_v_i >= d_u_v_0 - i or d_u_v_0 - d_u_v_i <= i
def linearized_distances():
    linearized_distance_constraints = []
    for edge in _edge_list:
        source = edge[0]
        target = edge[1]
        linearized_distance_constraints\
            .extend(linear_distance_constraints(source, target))
    return linearized_distance_constraints

def linear_distance_constraints(u, v):
    global _integer_variables
    linearized_distance_constraints = []
    relop = '<='
    distance_variable = "d_" + u + "_" + v + "_0"
    for i in range(1, _max_layer_size):
        right = str(i)
        linear_variable = "d_" + u + "_" + v + "_" + str(i)
        _integer_variables.append(linear_variable)
        left = ["+ " + distance_variable]
        left.append("- " + linear_variable)
        linearized_distance_constraints.append((left, relop, right))
    return linearized_distance_constraints

# @return constraints that will make "distance" variables d_u_v = d_u_v_0
# represent the absolute value of the difference between u's position on its
# layer and v's position on its layer; also adds the appropriate variables to
# the list _integer_variables; the sum of squares of these variables is the
# total non-verticality; see edge_nonverticalities for a linear version
def distance_definitions():
    global _integer_variables
    distance_constraints = []
    relop = '>='
    right = '0'
    for edge in _edge_list:
        source = edge[0]
        target = edge[1]
        source_layer = _node_dictionary[source][0]
        target_layer = _node_dictionary[target][0]
        source_position_variable = "p_" + source + "_" + source_layer
        target_position_variable = "p_" + target + "_" + target_layer
        distance_variable = "d_" + source + "_" + target + "_0"
        _integer_variables.append(distance_variable)
        left = ["+ " + distance_variable]
        left.append("+ " + source_position_variable)
        left.append("- " + target_position_variable)
        distance_constraints.append((left, relop, right))
        left = ["+ " + distance_variable]
        left.append("- " + source_position_variable)
        left.append("+ " + target_position_variable)
        distance_constraints.append((left, relop, right))
    return distance_constraints

# @return a set of constraints that define q_u_v = the (quadratic)
# nonverticality of edge uv, defined by the linearized distances as follows
#  q_u_v = d_u_v_0 + 2 d_u_v_1 + 2 d_u_v_2 + ... + 2 d_u_v_max
# This works because q_u_v is supposed to be d_u_v_0 squared and
#  d_u_v_i = d_u_v_0 - i; actually >= but it doesn't matter
# so we get sum_{i = 1 to d_u_v_0} i + sum_{i = 1 to d_u_v_0 - 1} i
# see linearized distances
def edge_nonverticalities():
    nonverticality_constraints = []
    for edge in _edge_list:
        source = edge[0]
        target = edge[1]
        nonverticality_constraints\
            .append(edge_nonverticality_constraint(source, target))
    return nonverticality_constraints

def edge_nonverticality_constraint(u, v):
    global _nonverticality_variables
    quadratic_variable = 'q_' + u + '_' + v
    _nonverticality_variables.append(quadratic_variable)
    relop = '='
    right = '0'
    prefix = 'd_' + u + "_" + v + "_"
    left = ['+ ' + prefix + '0']
    for i in range(1, _max_layer_size):
        left.append('+ 2 ' + prefix + str(i))
    left.append('- ' + quadratic_variable)
    return (left, relop, right)

# sets the variable vertical = sum of all q_u_v's
def total_nonverticality():
    relop = '='
    right = '0'
    left = ['+ ' + x for x in _nonverticality_variables]
    left.append('- vertical')
    _nonverticality_variables.append('vertical')
    return (left, relop, right)

# @return a list of constraints that represent lower bounds on the
# nonverticality of individual nodes, based on indegree and outdegree
# Note: we need to consider each edge from both directions
def verticality_lower_bounds():
    lower_bounds = []
    for node in _node_dictionary:
        up_neighbors = _node_dictionary[node][1]
        lower_bounds\
            .extend(verticality_bounds(node, up_neighbors))
        down_neighbors = _node_dictionary[node][2]
        lower_bounds\
            .extend(verticality_bounds(node, down_neighbors))
    return lower_bounds

def verticality_bounds(node, neighbors):
    relop = '>='
    bounds = []
    degree = len(neighbors)
    if degree > 1:
        for i in range(0, int(degree / 2)):
            right = str((int(degree / 2) - i)
                        * (int(degree / 2 + 1/2) - i))
            left = ["+ d_" + node + "_" + neighbor + "_" + str(i) 
                    for neighbor in neighbors]
            bounds.append((left, relop, right))
            left = ["+ d_" + neighbor + "_" + node + "_" + str(i) 
                    for neighbor in neighbors]
            bounds.append((left, relop, right))
    return bounds

# @return a list of lower bounds on verticality of complete bipartite graphs
# @param limit upper limit on number of nodes to consider
def bipartite_constraints(limit):
    if limit < 2:
        limit = _max_layer_size
    # for each layer except the last
    #   for each subset s of <= limit nodes in the layer
    #      if the intersection of upward neighbors forms a complete bipartite graph
    #           with at least 6 edges then
    #         form a complete bipartite constraint
    for layer in _layer_list[:-1]:
        layer_set = set(layer)
        # can quit if there are no small complete bipartite subgraphs
        larger_subgraphs_possible = True
        for subset_size in range(2, min(len(layer), limit) + 1):
            if subset_size > 2:
                larger_subgraphs_possible = False
            subset_list = [list(x)
                           for x in itertools.combinations(layer_set,
                                                           subset_size)]
            for subset in subset_list:
                print("subset =", subset)
                up_neighbors = [_node_dictionary[x][1] for x in subset]
                intersection = set.intersection(*[set(x) for x in up_neighbors])
                if len(intersection) * len(subset) >= 6:
                    larger_subgraphs_possible = True
                    print("intersection =", intersection) # for now
            if not larger_subgraphs_possible:
                break
    return []

# @return a single constraint for total stretch, i.e., stretch >= sum of
# stretch variables
def total_stretch_constraint():
    relop = '>='
    right = '0'
    _continuous_variables.append("stretch")
    left = ["+ stretch"]
    for stretch_variable in _stretch_variables:
        left.append("- " + stretch_variable)
    return (left, relop, right)

# @return a list of bottleneck stretch constraints, i.e., bn_stretch is >=
# each stretch variable.
def bottleneck_stretch_constraints():
    global _continuous_variables
    relop = '>='
    right = '0'
    _continuous_variables.append("bn_stretch")
    bottleneck_stretch_constraints = []
    for stretch_variable in _stretch_variables:
        left = ["+ bn_stretch", "-" + stretch_variable]
        bottleneck_stretch_constraints.append((left, relop, right))
    return bottleneck_stretch_constraints

# permutes the left hand side of each constraint as well as the order of the
# constraints in the list
def permute_constraints(constraints):
    random.shuffle(constraints)
    for constraint in constraints:
        random.shuffle(constraint[0])

# @return a string consisting of the elements of L separated by blanks and
# with a line break (and indentation) inserted between sublists of length
# max_length
def split_list(L, max_length):
    number_of_segments = int(math.ceil(len(L) / float(max_length)))
    output = ""
    for i in range(number_of_segments):
        segment = L[i * max_length:(i + 1) * max_length]
        output += ' '.join(segment)
        if i < number_of_segments - 1:
            output += '\n' + INDENT + INDENT
    return output

def date():
    date_pipe = os.popen( 'date -u "+%Y/%m/%d %H:%M:%S"' )
    return date_pipe.readlines()[0].strip()

def print_header():
    print("\\ " + ' '.join(sys.argv))
    print("\\ " + date())

def print_comments():
    for comment in _comments:
        print("\\ " + comment)

def print_quadratic_stretch_objective():
    quadratic_variables_squared = ["+" + x + "^2" for x in _raw_stretch_variables]
    print(INDENT + "[ " +  split_list(quadratic_variables_squared, MAX_TERMS_IN_LINE) + " ]/2")

# prints an objective representing total nonverticality The "[" and "]/2" at
# beginning and end of the list, respectively, are needed for quadratic
# expressions in CPLEX.
# Note: need to make coefficients = 2; CPLEX divides them by 2 for some
# reason, but only in the objective function.
def print_verticality_objective():
    print(INDENT + "[ " \
              + split_list(verticality_variables_squared, MAX_TERMS_IN_LINE) \
              + " ]/2" )

def print_constraint(constraint):
    (left, relop, right) = constraint
    print(INDENT + split_list(left, MAX_TERMS_IN_LINE), relop, right)

def print_constraints(constraints):
    for constraint in constraints:
        print_constraint(constraint)

# need to make the lower bound on raw stretch variables less than 0; -1 will work
def print_bounds_on_raw_stretch_variables():
    for variable in _raw_stretch_variables:
        print(INDENT + "-1 <= " + variable + " <= 1")

def print_variables():
    print("Binary")
    print(INDENT + split_list(list(_binary_variables), MAX_TERMS_IN_LINE))
    print("General")
    print(INDENT + split_list(list(_integer_variables), MAX_TERMS_IN_LINE))
    if _nonverticality_variables != []:
        print(INDENT + split_list(list(_nonverticality_variables), MAX_TERMS_IN_LINE))
    if _continuous_variables != []:
        print("Semi")
        print(INDENT + split_list(list(_continuous_variables), MAX_TERMS_IN_LINE))

def main():
    read_sgf(sys.stdin)
    # the following is not relevant for all objectives but couldn't hurt
    # makes logic a lot easier to do it up front
    compute_layer_factors()
    constraints = triangle_constraints()
    # always need to print values of position variables to allow translation
    # back to an sgf file that captures the optimum order
    if args.objective == 'vertical' or args.vertical != None:
        constraints.extend(position_constraints(False))
    else:
        constraints.extend(position_constraints(True))
    # always need to print at least one variable for each edge so that the
    # solution sgf file gets the edges right; in this case it's a silly constraint
    # that looks like an edge crossing itself - that's what sol2sgf.py expects
    constraints.extend(edges_for_output())
    if args.objective == 'total' or args.objective == 'bottleneck' \
            or args.total != None or args.bottleneck != None:
        constraints.extend(crossing_constraints())
    if args.objective == 'stretch' or args.objective == 'bn_stretch' \
            or args.stretch != None or args.bn_stretch != None \
            or args.objective == 'quadratic':
        constraints.extend(raw_stretch_constraints())
        raw_stretch_constraints_added = True
    if args.objective == 'stretch' or args.objective == 'bn_stretch' \
            or args.stretch != None or args.bn_stretch != None:
        constraints.extend(stretch_constraints())
    if args.objective == 'total' or args.total != None:
        constraints.append(total_constraint())
    if args.objective == 'bottleneck' or args.bottleneck != None:
        constraints.extend(bottleneck_constraints())
    if args.objective == 'stretch' or args.stretch != None:
        constraints.append(total_stretch_constraint())
    if args.objective == 'bn_stretch' or args.bn_stretch != None:
        constraints.extend(bottleneck_stretch_constraints())
    if args.objective == 'quadratic' and not raw_stretch_constraints_added:
        constraints.extend(raw_stretch_constraints())
    if args.objective == 'vertical' or args.vertical != None:
        constraints.extend(distance_definitions())
        constraints.extend(linearized_distances())
        constraints.extend(edge_nonverticalities())
        constraints.extend(verticality_lower_bounds())
        constraints.append(total_nonverticality())
        if args.bipartite != None:
            constraints.extend(bipartite_constraints(args.bipartite))
            sys.exit()

    # add specific constraints for each objective if appropriate
    if args.total != None:
        constraints.append((["+ total"], "<=", str(args.total)))
    if args.bottleneck != None:
        constraints.append((["+ bottleneck"], "<=", str(args.bottleneck)))
    if args.stretch != None:
        constraints.append((["+ stretch"], "<=", str(args.stretch)))
    if args.bn_stretch != None:
        constraints.append((["+ bn_stretch"], "<=", str(args.bn_stretch)))
    if args.vertical != None:
        constraints.append((["+ vertical"], "<=", str(args.vertical)))

    if args.seed != None:
        random.seed(args.seed)
        permute_constraints(constraints)

    print_header()
    print_comments()

    print("Min");
    if args.objective == 'quadratic':
        print_quadratic_stretch_objective()
    else:
        print(INDENT + args.objective)
    print("st")
    print_constraints(constraints)
    if _raw_stretch_variables != []:
        print("Bounds")
        print_bounds_on_raw_stretch_variables()
    print_variables()
    print("End")

main()

#  [Last modified: 2020 05 11 at 18:29:30 GMT]
