/**
 * @file sgf.h
 * @brief
 * Module for reading and writing files in .sgf format
 * @author Matt Stallmann
 * @date 2019/12/19
 */

/**
 * sgf format is as follows (blank lines are ignored):
 *    c comment line 1
 *    ...
 *    c comment line k
 *
 *    t graph_name nodes edges layers
 *
 *    n id_1 layer_1 position_1
 *    n id_2 layer_2 position_2
 *    ...
 *    n id_n layer_n position_n
 *
 *    e source_1 target_1
 *    ...
 *    e source_m target_m
 */

#ifndef SGF_H
#define SGF_H

#include<stdio.h>
#include<stdbool.h>

/**
 * structure for storing node information available in an sgf file
 */
struct sgf_node_struct { int id; int layer; int position; };

/**
 * structure for storing edge information available in an sgf file
 */
struct sgf_edge_struct { int source; int target };

/**
 * reads from 'in' past the comments
 */
void initSgf(FILE * in);

/**
 * places the name of the graph into buffer, which must be preallocated
 * @assume initSgf() has already been called (hence no need for the file ptr)
 */
void getNameFromSgfFile(char * buffer);

/**
 * @return number of nodes
 * @assume initSgf() and getNameFromSgfFile() have already been called
 */
int getNumberOfNodes();

/**
 * @return number of edges
 * @assume initSgf(), getNameFromSgfFile(), and getNumberOfNodes() have
 * already been called
 */
int getNumberOfEdges();

/**
 * @return number of layers
 * @assume initSgf()), getNameFromSgfFile(), getNumberOfNodes(), and
 * getNumberOfEdges() have already been called
 */
int getNumberOfLayers();

/**
 * put node id, layer, and position into the preallocated sgf_node
 * @return true if there is a next node
 * @assume the tag line has been read, i.e., getNameFromSgfFile(),
 * getNumberOfNodes(), getNumberOfEdges(), and getNumberOfLayers() have
 * already been called
 */
bool getNextNode(FILE * in, struct sgf_node_struct * sgf_node);

/**
 * put source and target into the preallocated edge
 * @return true if there is a next edge
 * @assume all nodes have been read from the file using getNumberOfNodes()
 */
bool getNextEdge(File * in, struct sgf_edge_struct * sgf_edge);

#endif

/*  [Last modified: 2019 12 16 at 20:44:57 GMT] */
