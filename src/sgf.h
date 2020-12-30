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
#include<stdlib.h>
#include<stdbool.h>

/**
 * Reads a graph in sgf format from the given stream
 */
void readSgf(FILE * sgf_stream);

/**
 * structure for storing information about node just read from file
 */
struct sgf_node_struct { int id; int layer; int position; } sgf_node;

/**
 * structure for storing information about edge just read from file
 */
struct sgf_edge_struct { int source; int target; } sgf_edge;

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
 * put node id, layer, and position into sgf_node
 * @return true if there is a next node
 * @assume the tag line has been read, i.e., getNameFromSgfFile(),
 * getNumberOfNodes(), getNumberOfEdges(), and getNumberOfLayers() have
 * already been called
 */
bool getNextNode();

/**
 * put source and target into sgf_edge
 * @return true if there is a next edge
 * @assume all nodes have been read from the file using getNumberOfNodes()
 */
bool getNextEdge();

/**
 * Writes the current graph and its ordering to an sgf file with the given name.
 * @param output_stream either a pointer to a file or stdout
 */
void writeSgf(FILE * output_stream);

#endif

/*  [Last modified: 2020 12 30 at 14:35:33 GMT] */
