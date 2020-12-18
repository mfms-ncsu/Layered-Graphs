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
 * reads from 'in' past the comments
 */
void initSgf(FILE * in) {
    in_stream = in;
    fgets(local_buffer, MAX_NAME_LENGTH, in_stream);
    while ( local_buffer[0] == 'c' ) {
        fgets(local_buffer, MAX_NAME_LENGTH, in_stream);
    }
    // assert: first char of local_buffer should be 't' at this point
}

/**
 * places the name of the graph into buffer, which must be preallocated
 * @assume local_buffer contents start with 't' 
 */
void getNameFromSgfFile(char * buffer) {
    sscanf(local_buffer, "t %s", buffer);
}

/**
 * @return number of nodes
 * @assume name has been read from local buffer
 */
int getNumberOfNodes() {
    int retval;
    sscanf(local_buffer, "%d", &retval);
    return retval;
}

/**
 * @return number of edges
 */
int getNumberOfEdges() {
    int retval;
    sscanf(local_buffer, "%d", &retval);
    return retval;
}

/**
 * @return number of layers
 */
int getNumberOfLayers() {
    int retval;
    sscanf(local_buffer, "%d", &retval);
    return retval;
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
    fgets(local_buffer, MAX_NAME_LENGTH, in_stream);
    if ( strlen(local_buffer) > 0 && local_buffer[0] == 'e' ) {
        sscanf(local_buffer, "e %d %d",
               &(sgf_edge.source), &(sgf_edge.target));
        return true;
    }
    return false;
}

/*  [Last modified: 2020 12 18 at 17:34:18 GMT] */
