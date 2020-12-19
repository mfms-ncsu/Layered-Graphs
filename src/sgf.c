/**
 * @file sgf.c
 * @brief
 * Implementation of module for reading and writing files in .sgf format
 * @author Matt Stallmann
 * @date 2019/12/19
 */

#include<string.h>
#include "sgf.h"
#include "defs.h"

/**
 * stores the file stream
 */
FILE * in_stream;

/**
 * stores current line while reading file
 */
static char local_buffer[MAX_NAME_LENGTH];

/**
 * storage for items on the tag line
 */
static char local_name[MAX_NAME_LENGTH];
static int local_nodes;
static int local_edges;
static int local_layers;

/**
 * reads from 'in' past the comments
 */
void initSgf(FILE * in) {
    in_stream = in;
    fgets(local_buffer, MAX_NAME_LENGTH, in_stream);
    while ( local_buffer[0] == 'c' ) {
        fgets(local_buffer, MAX_NAME_LENGTH, in_stream);
    }
    // assert: first char of local_buffer should be 't' at this point
    sscanf(local_buffer, "t %s %d %d %d",
           local_name, &local_nodes, &local_edges, &local_layers);
}

/**
 * places the name of the graph into buffer, which must be preallocated
 * @assume local_buffer contents start with 't' 
 */
void getNameFromSgfFile(char * name_buffer) {
    strcpy(name_buffer, local_name);
}

/**
 * @return number of nodes
 * @assume name has been read from local buffer
 */
int getNumberOfNodes() {
    return local_nodes;
}

/**
 * @return number of edges
 */
int getNumberOfEdges() {
    return local_edges;
}

/**
 * @return number of layers
 */
int getNumberOfLayers() {
    return local_layers;
}

/**
 * put node id, layer, and position into the preallocated sgf_node
 * @return true if there is a next node
 */
bool getNextNode() {
    fgets(local_buffer, MAX_NAME_LENGTH, in_stream);
    if ( strlen(local_buffer) > 0 && local_buffer[0] == 'n' ) {
        sscanf(local_buffer, "n %d %d %d",
               &(sgf_node.id), &(sgf_node.layer), &(sgf_node.position));
        return true;
    }
    return false;
}

/**
 * put source and target into the preallocated edge
 * @return true if there is a next edge
 */
bool getNextEdge() {
    char * success = fgets(local_buffer, MAX_NAME_LENGTH, in_stream);
    if ( success != NULL && strlen(local_buffer) > 0 && local_buffer[0] == 'e' ) {
        sscanf(local_buffer, "e %d %d",
               &(sgf_edge.source), &(sgf_edge.target));
        return true;
    }
    return false;
}

/*  [Last modified: 2020 12 19 at 15:25:55 GMT] */
