#! /usr/bin/env python3

import sys
import os

class LayeredGraph:
    """
    Maintains all information about a proper layered graph:
    - list of node id's (in order they were added)
    - mapping from a node id (a string, usually encoding an integer)
      to information about a node
    - layer and position of each node
    - list of (id's of) neighbors above and below each node
    - list of (id's of) nodes on each layer
    - list of edges (in order they were added)
    - graph name and comments
    """

    def __init__(self):
        self.nodeIds = []
        self.layerOfNode = {}
        self.positionOfNode = {}
        self.neighborsAbove = {}
        self.neighborsBelow = {}
        # layers[i] will be a list of id's of nodes on layer i, not in any
        # particular order
        self.layers = []
        # each item in edges is a tuple of the form (source,target), where
        # source is on the lower numbered layer
        self.edges = []
        # nodeAt[(L,p)] will be the node at position p on layer L, if it exists
        self.nodeAt = {}
        # a list of comments -- all formats have these
        self.comments = []
        # name of the graph, often the base name of the file from which it comes
        self.name = ""

    def upDegree(self, node_id):
        """
        @return the number of neighbors the node has on the level above
        """
        return len(self.neighborsAbove[node_id])
    
    def downDegree(self, node_id):
        """
        @return the number of neighbors the node has on the level below
        """
        return len(self.neighborsBelow[node_id])
    
    def readSgf(self, stream):
        """
        reads an sgf file from the stream and sets up data for this instance
        accordingly
        """
        line = self.skip_comments(stream)
        # for now, assume next line begins with 't'; do error checking later
        # since the number of nodes and edges is implicit, these can be ignored
        self.name = line.split()[1]
        line = self.read_nonblank(stream)
        while (line):
            node_or_edge = line.split()[0]
            if node_or_edge == 'n':
                self.process_node(line)
            elif node_or_edge == 'e':
                self.process_edge(line)
            # otherwise error (ignore for now)
            line = self.read_nonblank(stream)

    def skip_comments(self, stream):
        """
        reads and skips lines that begin with 'c' and adds them to
        the list of comments, one element per comment line;
        assumes that there is a blank between the 'c' and the comment
        @return the first line that is not a comment line
        """
        line = self.read_nonblank(stream)
        while ( line.split()[0] == 'c' ):
            self.comments.append(line.strip().lstrip("c "))
            line = self.read_nonblank(stream)
        return line

    def read_nonblank(self, stream):
        """
        @return the next non-blank line from the stream
        """
        line = stream.readline()
        while ( line and line.strip() == "" ):
            line = stream.readline()
        return line

    def process_node(self, line):
        """
        @param line a string of the form 'n id layer position'
        @post data structures are up to date given the new information about
        the node whose id is given
        """
        line_fields = line.split()
        node_id = line_fields[1]
        self.nodeIds.append(node_id)
        layer = int(line_fields[2])
        position_in_layer = int(line_fields[3])
        self.layerOfNode[node_id] = layer
        self.positionOfNode[node_id] = position_in_layer
        self.nodeAt[(layer, position_in_layer)] = node_id
        self.add_node_to_layer(node_id, layer)

    def add_node_to_layer(self, node_id, layer):
        """
        adds the node to the given layer, creating empty lists for any
        missing layers, i.e., if the highest-numbered layer k is < layer,
        then layers k+1,...,layer need to be added as empty lists
        """
        layer_to_create = len(self.layers)
        while layer_to_create <= layer:
            self.layers.append([])
            layer_to_create += 1
        self.layers[layer].append(node_id)

    def process_edge(self, line):
        """
        @param line a string of the form 'id source target'
        @post data structures are up to date given the new information about
        the edge (source,target)
        """
        line_fields = line.split()
        source = line_fields[1]
        target = line_fields[2]
        # for now we assume that the input has source and target in the right
        # order
        self.edges.append((source,target))
        self.neighborsAbove[source] = target
        self.neighborsBelow[target] = source

    def writeSgf(self, stream):
        """
        writes the layered graph in sgf format on the given stream, format is -

        lines beginning with n give info about nodes,
        those beginning with e give info about edges
        comments are used to document the provenance of the graph
        c comment line 1
        ...
        c comment line k

        t graph_name
        n id_1 layer_1 position_1
        n id_2 layer_2 position_2
        ...
        n id_n layer_n position_n
        e source_1 target_1
        ...
        e source_m target_m
        """
        # write the comments first
        for comment in self.comments:
            stream.write("c {}\n".format(comment))
        # write the name, number of nodes, and number of edges in sgf format
        stream.write("t {} {} {}\n".format(self.name,
                                           len(self.nodeIds),
                                           len(self.edges)))
        # write information about each node in sgf format
        for node_id in self.nodeIds:
            stream.write("n {} {} {}\n".format(node_id,
                                               self.layerOfNode[node_id],
                                               self.positionOfNode[node_id]))
        # write information about the edges
        for edge in self.edges:
            stream.write("e {} {}\n".format(edge[0], edge[1]))

# test program: reads an sgf file and prints it (stdin -> stdout)
if __name__ == '__main__':
    lg = LayeredGraph()
    lg.readSgf(sys.stdin)
    lg.writeSgf(sys.stdout)
    
#  [Last modified: 2018 06 20 at 18:08:22 GMT]
