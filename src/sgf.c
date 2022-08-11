/**
 * @file sgf.c
 * @brief
 * Implementation of module for reading and writing files in .sgf format
 * Deals with details of sgf format
 * @author Matt Stallmann
 * @date 2019/12/19
 */

#include<string.h>
#include<ctype.h>
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

// these are the numbers in the 't' line of the sgf file,
// which may not correspond to reality
static int num_nodes, num_edges, num_layers;

/**
 * stores current line while reading file
 */
static char local_buffer[MAX_NAME_LENGTH];

/**
 * was the last read successful, NULL if not
 */
static char * success;

static int line_number = 0;

/**
 * works like fgets but trims off any trailing newline
 */
char * get_line(char *s, int n, FILE * stream) {
    // true if the previous "line" had a '\n' terminator
    static bool previous_eol = true;
    char * return_value = fgets(s, n, stream);
    size_t last_position = strlen(s) - 1;
    // increment line number only if previous line ended with \n
    if ( previous_eol ) 
        line_number++;
    // if first char not = \0 and last char = \n, replace last char with \0
    if (*s && s[last_position] == '\n') {
        s[last_position] = '\0';
        previous_eol = true;
    }
    else {
        previous_eol = false;
    }
    return return_value;
}

/**
 * @return true iff the string is blank (all whitespace)
 */
bool is_blank(const char *s) {
    while ( *s != '\0' ) {
        if ( ! isspace(*s) ) return false;
        s++;
    }
    return true;
}

/**
 * reads from 'in' past the comments and stores the comments (see graph.h)
 */
void initSgf(FILE * in) {
    in_stream = in;
    number_of_nodes = 0;
    number_of_edges = 0;
    startAddingComments();
    success = get_line(local_buffer, MAX_NAME_LENGTH, in_stream);
    while ( success != NULL && local_buffer[0] != 't' ) {
        if ( is_blank(local_buffer) ) {
           success = get_line(local_buffer, MAX_NAME_LENGTH, in_stream);
           continue; 
        }
        if ( local_buffer[0] != 'c' ) {
            fprintf(stderr, "*** FATAL, line %d: expected to start with 'c' but is '%s'\n",
                    line_number, local_buffer);
            abort();
        }
        addComment(local_buffer + 2, true); /* ignore the 'c' */
        success = get_line(local_buffer, MAX_NAME_LENGTH, in_stream);
    }
    // assert: first char of local_buffer should be 't' at this point
    if ( success == NULL ) {
        fprintf(stderr, "*** FATAL, line %d: no graph information.\n", line_number);
        abort();
    }
    int num_read = sscanf(local_buffer, "t %s %d %d %d",
                          graph_name, &num_nodes, &num_edges, &num_layers);
    if ( num_read != 4 ) {
        fprintf(stderr, "*** FATAL, line %d: bad header information '%s'\n",
               line_number, local_buffer);
        abort();
    }
}

/**
 * Creates the struct for each node using makeNumberedNode() in
 * graph_io.c and adds (a pointer to) it to the master list
 */
void readSgfNodes(FILE * in_stream) {
#ifdef DEBUG
    printf("-> readSgfNodes\n");
#endif
    success = get_line(local_buffer, MAX_NAME_LENGTH, in_stream);
    while ( success != NULL && local_buffer[0] != 'e' ) {
        if ( is_blank(local_buffer) ) {
           success = get_line(local_buffer, MAX_NAME_LENGTH, in_stream);
           continue; 
        }
        if ( local_buffer[0] != 'n' ) {
            fprintf(stderr, "*** FATAL, line %d expected to start with 'n' but is '%s'\n",
                    line_number, local_buffer);
            abort();
        }
        int id, layer, position;
        int num_values = sscanf(local_buffer, "n %d %d %d",
                                &id, &layer, &position);
        if ( num_values != 3 ) {
            fprintf(stderr, "*** FATAL line %d, incomplete node information '%s'\n",
                    line_number, local_buffer);
            abort();
        }
        if ( layer >= number_of_layers ) {
            // recall 0-based indexing on layers
            number_of_layers = layer + 1;
        }
        if ( number_of_nodes > num_nodes ) {
            master_node_list
             = (Nodeptr *) realloc(master_node_list,
                                   (number_of_nodes + 1) * sizeof(Nodeptr));
        }
        makeNumberedNode(id, layer, position);
        number_of_nodes++;
        success = get_line(local_buffer, MAX_NAME_LENGTH, in_stream);
    }
    if ( num_nodes != number_of_nodes ) {
        fprintf(stderr, "*** Warning: 't' line says %d nodes, but there are %d nodes\n",
                num_nodes, number_of_nodes);
    }
    // check to see if number of layers on 't' line matches reality
    // and issue warning if not; also need to fix size of layer array
    if ( num_layers != number_of_layers ) {
        fprintf(stderr, "*** Warning: 't' line says %d layers, but there are %d layers\n",
                num_layers, number_of_layers);
        layers
           = (Layerptr *) realloc(layers, number_of_layers * sizeof(Layerptr));
    }

#ifdef DEBUG
    printf("<- readSgfNodes, number_of_nodes = %d, num_nodes = %d\n",
            number_of_nodes, num_nodes);
#endif
}

/**
 * Creates the struct for each edge and adds (a pointer to) it to the
 * master list; uses addEdge to do sanity checks and retrieve node
 * pointers
 */
void readSgfEdges(FILE * in_stream) {
#ifdef DEBUG
    printf("-> readSgfEdges\n");
#endif
    while ( success != NULL ) {
        if ( is_blank(local_buffer) ) {
           success = get_line(local_buffer, MAX_NAME_LENGTH, in_stream);
           continue; 
        }
        if ( local_buffer[0] != 'e' ) {
            fprintf(stderr, "*** FATAL, line %d expected to start with 'e' but is '%s'\n",
                    line_number, local_buffer);
            abort();
        }
        int source, target;
        int num_values = sscanf(local_buffer, "e %d %d",
                                &source, &target);
        if ( num_values != 2 ) {
            fprintf(stderr, "*** FATAL, line %d: incomplete edge information '%s'\n",
                    line_number, local_buffer);
            abort();
        }
        char source_name[MAX_NAME_LENGTH];
        char target_name[MAX_NAME_LENGTH];
        strcpy(source_name, nameFromId(source));
        strcpy(target_name, nameFromId(target));
        number_of_edges++;
        if ( number_of_edges > num_edges ) {
            master_edge_list
             = (Edgeptr *) realloc(master_edge_list,
                                   (number_of_edges + 1) * sizeof(Edgeptr));
        }
        // addEdge() increments number_of_edges, so no need to do it here
        addEdge(source_name, target_name);
        success = get_line(local_buffer, MAX_NAME_LENGTH, in_stream);
    }
    if ( num_edges != number_of_edges ) {
        fprintf(stderr, "*** Warning: 't' line says %d edges, but there are %d edges\n",
                num_edges, number_of_edges);
    }
}

/**
 * Allocates the hash table to correspond to the known number of nodes
 * and puts each node on the master node list into it
 * Assumes that number_of_nodes is correct and master_node_list contains
 * all of the nodes 
 */
static void insert_nodes_in_hash_table(void) {
    initHashTable(number_of_nodes);
    for ( int i = 0; i < number_of_nodes; i++ ) {
        Nodeptr current_node = master_node_list[i];
        insertInHashTable(current_node->name, current_node);
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
 *      (d) add it to the master node list
 * [At this point the correct number of nodes and layers is known]
 *  2'. Allocate correct amount of space in hash table and
 *        insert all nodes into it
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
 *  5'. Sort each layer by position and check for duplicates
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
    master_node_list = (Nodeptr *) calloc(num_nodes, sizeof(Nodeptr));
    master_edge_list = (Edgeptr *) calloc(num_edges, sizeof(Edgeptr));
    layers = (Layerptr *) calloc(num_layers, sizeof(Layerptr));
    readSgfNodes(sgf_stream);
    insert_nodes_in_hash_table();
    readSgfEdges(sgf_stream);
    allocateLayers();
    addNodesToLayers();
//    sort_all_layers_by_position();
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
