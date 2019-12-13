/**
 * @file sgf.c
 * @brief
 * Implementation of module for reading and writing files in .sgf format
 * @author Matt Stallmann
 * @date 2019/12/19
 */

#include "sgf.h"
#include "defs.h"

/**
 * reads from 'in' past the comments
 */
void initSgf(FILE * in) {
    fgets
}

/**
 * places the name of the graph into buffer, which must be preallocated
 */
void getNameFromSgfFile(FILE * in, char * buffer);

/**
 * @return number of nodes
 */
int getNumberOfNodes(FILE * in);

/**
 * @return number of edges
 */
int getNumberOfEdges(FILE * in);

/**
 * @return number of layers
 */
int getNumberOfLayers(FILE * in);

/**
 * put node id, layer, and position into the preallocated sgf_node
 * @return true if there is a next node
 */
bool getNextNode(FILE * in, struct sgf_node_struct * sgf_node);

/**
 * put source and target into the preallocated edge
 * @return true if there is a next edge
 */
bool getNextEdge(File * in, struct sgf_edge_struct * sgf_edge);

#endif

/*  [Last modified: 2019 12 13 at 21:02:09 GMT] */
