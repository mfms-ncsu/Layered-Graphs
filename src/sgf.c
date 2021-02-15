/**
 * @file sgf.c
 * @brief
 * Implementation of module for reading and writing files in .sgf format
 * Deals with details of sgf format
 * @author Matt Stallmann
 * @date 2019/12/19
 */

#include<string.h>
#include "sgf.h"
#include "defs.h"
#include "graph.h"
#include "graph_io.h"
#include "hash.h"

/**
 * stores a long string of comments separated by '\n's
 */
char * comments = NULL;

/**
 * stores the file stream
 */
FILE * in_stream;

/**
 * stores current line while reading file
 */
static char local_buffer[MAX_NAME_LENGTH];

/**
 * was the last read successful, NULL if not
 */
static char * success;

/**
 * reads from 'in' past the comments and stores the comments (see graph.h)
 */
void initSgf(FILE * in) {
    in_stream = in;
    startAddingComments();
    success = fgets(local_buffer, MAX_NAME_LENGTH, in_stream);
    while ( success != NULL && local_buffer[0] == 'c' ) {
        addComment(local_buffer + 2, false); /* ignore the 'c' */
        success = fgets(local_buffer, MAX_NAME_LENGTH, in_stream);
    }
    // assert: first char of local_buffer should be 't' at this point
    if ( success == NULL ) {
        printf("FATAL: no graph information.\n");
    }
    int num_read = sscanf(local_buffer, "t %s %d %d %d",
                          graph_name, &number_of_nodes, &number_of_edges, &number_of_layers);
    if ( num_read != 4 ) {
        printf("FATAL: bad header information '%s'\n", local_buffer);
    }
}

/**
 * Creates the struct for each node using makeNumberedNode() in
 * graph_io.c and adds (a pointer to) it to the master list
 */
void readSgfNodes(FILE * in_stream) {
    success = fgets(local_buffer, MAX_NAME_LENGTH, in_stream);
    while ( success != NULL && strlen(local_buffer) > 0 && local_buffer[0] == 'n' ) {
        int id, layer, position;
        int num_values = sscanf(local_buffer, "n %d %d %d",
                                &id, &layer, &position);
        if ( num_values != 3 ) {
            printf("FATAL, incomplete node information '%s'\n", local_buffer);
            abort();
        }
        makeNumberedNode(id, layer, position);
        success = fgets(local_buffer, MAX_NAME_LENGTH, in_stream);
    }
}

/**
 * Creates the struct for each edge and adds (a pointer to) it to the
 * master list; uses addEdge to do sanity checks and retrieve node
 * pointers
 */
void readSgfEdges(FILE * in_stream) {
    while ( success != NULL && strlen(local_buffer) > 0 && local_buffer[0] == 'e' ) {
        int source, target;
        int num_values = sscanf(local_buffer, "e %d %d",
                                &source, &target);
        if ( num_values != 2 ) {
            printf("FATAL, incomplete edge information %s\n", local_buffer);
            abort();
        }
        char source_name[MAX_NAME_LENGTH];
        char target_name[MAX_NAME_LENGTH];
        strcpy(source_name, nameFromId(source));
        strcpy(target_name, nameFromId(target));
        addEdge(source_name, target_name);
        success = fgets(local_buffer, MAX_NAME_LENGTH, in_stream);
    }
}

/**
 * Input algorithm for sgf files:
 *  1. Read comments and header information
 *      - allocate hash table
 *      - allocate master lists for nodes, edges, and layers
 *  2. Read nodes; for each node
 *      (a) create a struct for it (and pointer)
 *      (b) fill in name (id as text), id, layer, position
 *      (c) increment the number of nodes for its layer
 *      (d) put it in the hash table so the ptr can be gotten from id
 *      (e) add it to the master node list
 *  3. Read edges; for each edge
 *      (a) create a struct for it (and pointer)
 *      (b) retrieve (ptrs to) endpoints from the hash table
 *      (c) fill in source and target, checking for layers
 *      (d) increment up and down degrees for the endpoints
 *      (e) add it to the master edge list
 *  4. Allocate the node list for each layer; number of nodes is known
 *  5. Traverse the master node list; for each node
 *      - allocate arrays for up and down edges
 *      - add the node to its layer
 *  6. Traverse the master edge list; for each edge
 *      - add it to the arrays for up and down edges of endpoints
 *  7. Deallocate hash table
 *
 * Notes
 *  - use FILE * instead of file name; this allows use of pipes to run
 * multiple heuristics in sequence
 *  - functions in sgf.[ch] are lightweight; each requires that the
 * client allocate and deallocate a struct for the relevant info (or
 * use a static one throughout)
 */

void readSgf(FILE * sgf_stream) {
    initSgf(sgf_stream);
    initHashTable(number_of_nodes);
    master_node_list = (Nodeptr *) calloc(number_of_nodes, sizeof(Nodeptr));
    master_edge_list = (Edgeptr *) calloc(number_of_edges, sizeof(Edgeptr));
    layers = (Layerptr *) calloc(number_of_layers, sizeof(Layerptr));
    allocateLayers();
    readSgfNodes(sgf_stream);
    readSgfEdges(sgf_stream);
    // Note: the function below is called as part of addNodesToLayers()
    //    allocateNodeListsForLayers();
    addNodesToLayers();
    number_of_isolated_nodes = countIsolatedNodes();
    removeHashTable();
}

static void writeSgfComments(FILE * output_stream) {
    // write each line of the comment string separately, preceded by "c "
    startGettingComments();
    char buffer[MAX_NAME_LENGTH];
    while ( getNextComment(buffer) != NULL ) {
        fprintf(output_stream, "c %s\n", buffer);
    }
}

static void writeSgfTagLine(FILE * output_stream) {
    fprintf(output_stream, "t %s %d %d %d\n",
            graph_name, number_of_nodes, number_of_edges, number_of_layers);
}

static void writeSgfNodes(FILE * output_stream) {
    for ( int i = 0; i < number_of_nodes; i++ ) {
        Nodeptr node = master_node_list[i];
        fprintf(output_stream, "n %d %d %d\n",
                node->id, node->layer, node->position);
    }
}

static void writeSgfEdges(FILE * output_stream) {
    for ( int i = 0; i < number_of_edges; i++ ) {
        Edgeptr edge = master_edge_list[i];
        fprintf(output_stream, "e %d %d\n",
                edge->down_node->id, edge->up_node->id);
    }
}

void writeSgf(FILE * output_stream) {
    writeSgfComments(output_stream);
    writeSgfTagLine(output_stream);
    writeSgfNodes(output_stream);
    writeSgfEdges(output_stream);
}

/*  [Last modified: 2021 02 15 at 20:33:00 GMT] */
