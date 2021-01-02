/**
 * @file graph_io.h
 * @brief Definition of functions for reading and writing graphs.
 * @author Matt Stallmann, based on Saurabh Gupta's crossing heuristics
 * implementation.
 * @date 2008/12/19
 * $Id: graph_io.h 90 2014-08-13 20:31:25Z mfms $
 */

#include<stdbool.h>
#include<stdio.h>

#ifndef GRAPH_IO_H
#define GRAPH_IO_H

#include"graph.h"
#include"sgf.h"

/**
 * utility function that converts a number to a string
 * @return a pointer to the buffer - have to use strcpy to use the
 * string since buffer contents will change
 */
char * nameFromId(int id);

/**
 * @return number of nodes that have no incoming or outgoing edges
 */
int countIsolatedNodes();

/**
 * Creates an ord file name from the graph name, preprocessor and heuristic.
 * @param output_file_name a buffer for the file name to be created, assumed
 * to be big enough
 * @param preprocessor_arg the preprocessor as an arg so no reliance on global
 * @param heuristic_arg the heuristic as an arg so no reliance on global
 * @param appendix a string that is attached just before the
 * extension, usually the objective
 * @param extension either ".ord" or ".sgf" depending on format of output
 */
void createOutputFileName( char * output_file_name,
                           const char * preprocessor_arg,
                           const char * heuristic_arg,
                           const char * appendix,
                           const char * extension );

/**
 * writes an output file recording the current ordering of layers;
 * the file extension and format are based on global variables
 * write_ord and write_sgf; the tag parameters are used to create the
 * file name
 * @param objective_tag a string representing the objective
 */
void writeFile(const char * objective_tag);

/**
 * Adds an edge to the graph; (currently) used by both sgf and dot
 * input.
 * Names come directly from the dot file; for sgf, they are strings
 * representing numbers.
 * Although instances usually direct edges from lower to higher layers,
 * no such assumption is made here.  A fatal error occurs if the nodes are
 * not on adjacent layers.
 * Also (re)allocates the adjacency lists for the endpoints and adds
 * the edge to them while incrementing the number
 */
void addEdge(const char * source, const char * target);

/**
 * Creates a layer struct for each layer, assuming array 'layers' is allocated
 */
void allocateLayers(void);

/**
 * Allocates a node list of the right length (number of nodes) for
 * each layer.
 */
void allocateNodeListsForLayers(void);

/**
 * Adds each node to its layer.
 * Uses master_node_list and assumes the node lists have been allocated.
 */
void addNodesToLayers(void);

/**
 * Reads the graph from the given dot and ord files, specified by their names.
 * Each file is read twice so that arrays can be allocated to the correct
 * size on the first pass.
 * Also initializes all graph-related data structures and global variables.
 */
void readDotAndOrd( const char * dot_file, const char * ord_file );

/**
 * Prints the graph in a verbose format on standard output for debugging
 * purposes. May also be used for piping to a graphical trace later.
 */
void printGraph();

/**
 * Writes the current layer orderings in ord format to the out stream
 * @todo move this to ord.[ch]
 */
void writeOrd(FILE * out);

/**
 * Writes a dot file with the given name.
 * @param dot_file_name the output file name (including .dot extension)
 * @param graph_name name of graph
 * @param header_information what will go into the file before the '{' that
 * starts the list of edges (including the graph name)
 * @param edge_list array of (indices of) the edges to be written
 * @param edge_list_length length of the edge list
 */
void writeDot(const char * dot_file_name,
               const char * graph_name,
               const char * header_information,
               const Edgeptr * edge_list,
               int edge_list_length
               );

/**
 * Creates a new node with the given id number
 *   and performs 2. (a)-(e) above
 * @param id the id number of the node
 * @param layer the layer of the node
 * @param position the position of the node on its layer
 * @return (a pointer to) the newly created node
 */
Nodeptr makeNumberedNode(int id, int layer, int position);

/**
 * Initializes the comments string to be empty in preparation for
 * adding comments.
 */
void startAddingComments(void);

/**
 * Adds the given string as a comment to the comments string
 * @param needs_eol true if an end of line character should be added
 *        if comment comes from input via fgets, it will already be present
 */
void addComment(const char * comment, bool needs_eol);

/**
 * Initializes processing of the comments string; called at the
 * beginning of a sequence of getComment calls
 */
void startGettingComments(void);

/**
 * Puts the next comment into the comment buffer
 * @return a pointer to the beginning of the comment
 *         or NULL if there is no next comment
 */
char * getNextComment(char * comment_buffer);

#endif

/*  [Last modified: 2021 01 02 at 21:05:45 GMT] */
