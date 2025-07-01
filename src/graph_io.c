/**
 * @file graph_io.c
 * @brief Implementation of functions that create graph structures from input
 * .dot and .ord files. 
 * @author Matt Stallmann, based on Saurabh Gupta's crossing heuristics
 * implementation.
 * @date 2008/12/19
 * @todo most of these routines are relevant to dot and ord files only
 */

#include"graph.h"
#include"hash.h"
#include"defs.h"
#include"dot.h"
#include"ord.h"
#include"sgf.h"
#include"graph_io.h"

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<assert.h>

#define MIN_LAYER_CAPACITY 1 

Nodeptr * master_node_list;
Edgeptr * master_edge_list;
int number_of_nodes = 0;
int number_of_layers = 0;
int max_layer_width = 0;
int max_width_layer = -1; // unknown at this point
int number_of_edges = 0;
int number_of_isolated_nodes = 0;
Layerptr * layers = NULL;
char graph_name[MAX_NAME_LENGTH];

/**
 * write_ord_output and write_sgf_output are based on the nature
 * of the input
 */
bool write_ord_output = false;
bool write_sgf_output = false;

// initial allocated size of layer array (will double as needed)
static int layer_capacity = MIN_LAYER_CAPACITY;

// used when creating a name from an integer id - only needed for ord files
static char name_buffer[MAX_NAME_LENGTH];

/******* simple utilities for printing parts of the graph, mostly for debugging *********/
/********** more complex versions, with names writeX() at the end ************/

void printNode(FILE * stream, Nodeptr node) {
    fprintf(stream, " {#%d", node->id);
    fprintf(stream, " (%d,%d)", node->layer_index, node->horizontal_position);
    fprintf(stream, " v[");
    Edgeptr * down_edges = node->down_edges;
    for ( int j = 0; j < node->down_degree; j++ ) {
      int down_neighbor_id = down_edges[j]->down_node->id;
      fprintf(stream, " %d", down_neighbor_id);
    }
    fprintf(stream, " ]");
    fprintf(stream, " ^[");
    Edgeptr * up_edges = node->up_edges;
    for ( int j = 0; j < node->up_degree; j++ ) {
      int up_neighbor_id = up_edges[j]->up_node->id;
      fprintf(stream, " %d", up_neighbor_id);
    }
    fprintf(stream, " ]");
    fprintf(stream, " }"); 
}

void printEdge(FILE * stream, Edgeptr edge) {
  fprintf(stream, "{(%d->%d)", edge->down_node->id, edge->up_node->id);
  fprintf(stream, " x = %d, v = %d}", edge->crossings, edge->nonverticality);
}

void printNodesOnLayer(FILE * stream, int layer_number) {
  Nodeptr * nodes = layers[layer_number]->nodes;
  fprintf(stream, "layer: %d", layer_number);
  for ( int i = 0; i < layers[layer_number]->number_of_nodes; i++) {
    Nodeptr node = nodes[i];
    fprintf(stream, " "); printNode(stream, node);
  }
  fprintf(stream, "\n");
}

/**
 * @brief Get the base name of the file and copy it into the buffer
 */
void getBaseName(char * buffer, const char * file_name) {
  char * last_slash = strrchr(file_name, '/');
  char * last_dot = strrchr(file_name, '.');
  if ( last_dot == NULL ) {
    fprintf(stderr, "*** FATAL ERROR: file name %s has no extension\n", file_name);
    exit(EXIT_FAILURE);
  }
  int basename_length = (last_slash != NULL)
      ? strlen(last_slash) - strlen(last_dot) - 1
      : strlen(file_name) - strlen(last_dot);
  if ( basename_length > MAX_NAME_LENGTH - 1 ) {
      fprintf(stderr, "*** Warning: Base name longer than unix line length, truncating to 511\n");
      basename_length = 511;
  }
  if ( last_slash != NULL ) {
    strncpy(buffer, file_name, basename_length);
  }
  else {
    strncpy(buffer, file_name, basename_length);
  }
}

/**
 * utility function that converts a number to a string
 * @return a pointer to the buffer - have to use strcpy to use the
 * string since buffer contents will change
 */
char * nameFromId(int id) {
    sprintf(name_buffer, "%d", id);
    return name_buffer;
}

/**
 * @brief The (base) name of of the output file, which has tags for preprocessor and heuristic
 * @todo make this interior to createOutputBase() and return a pointer to it
 */
static char complete_output_file_base_name[MAX_NAME_LENGTH];

/**
 * @brief creates a base name for an output file that includes information about
 *        the preprocessor an heuristic used;
 *      the output base is stored in a global array for use when needed
 * 
 * @param objective_tag a tag indicating the objective function optimized in the output
 */
static void createOutputBase(const char * output_base_name, const char * objective_tag) {
#ifdef DEBUG
  fprintf(stderr, "-> createOutputBase(%s, %s)\n",
          output_base_name, objective_tag);
#endif
  strcpy( complete_output_file_base_name, output_base_name );
  strcat( complete_output_file_base_name, "-" );
  fprintf(stderr, "after adding -\n");
  if ( preprocessor != NULL ) {
    strcat( complete_output_file_base_name, preprocessor );
  }
  if( preprocessor != NULL && heuristic != NULL != 0 )
    strcat( complete_output_file_base_name, "+" );
  strcat( complete_output_file_base_name, heuristic );
  strcat( complete_output_file_base_name, "-" );
  strcat( complete_output_file_base_name, objective_tag );
#ifdef DEBUG
  fprintf(stderr, "<- createOutputBase, base = %s\n", complete_output_file_base_name);
#endif
}

/**
 * @brief Creates a file name based on information about objectives and type of file
 * 
 * @param output_file_name stores the return value
 * @param output_base_name the base name of the file, usually the input base name,
 *                         unless the -w option specifies otherwise
 * @param objective_tag a one or two letter tag for the objective function
 * @param extension either sgf or ord - will eventually default to sgf
 */
static void createOutputFileName(char * output_file_name,
                                 const char * output_base_name,
                                 const char * objective_tag,
                                 const char * extension) {
#ifdef DEBUG
  fprintf(stderr, "-> createOutputFileName(%s, %s, %s)\n",
          output_base_name, objective_tag, extension);
#endif
  createOutputBase(output_base_name, objective_tag);
  // Note: complete_output_file_base_name is global, created by createOutputBase()
  strcpy(output_file_name, complete_output_file_base_name);
  strcat(output_file_name, extension);
#ifdef DEBUG
  fprintf(stderr, "<- createOutputFileName, file name = %s\n", output_file_name);
#endif
}

void writeFile(const char * output_base_name, const char * objective_tag) {
    char output_file_name[MAX_NAME_LENGTH];
    char * extension = NULL;
    if ( write_sgf_output ) extension = ".sgf";
    else if ( write_ord_output ) extension = ".ord";
    createOutputFileName(output_file_name, output_base_name, objective_tag, extension);
    FILE * out_stream = fopen(output_file_name, "w");
    if( out_stream == NULL ) {
        fprintf(stderr, "Unable to open file %s for output\n", output_file_name);
        exit( EXIT_FAILURE );
    }
    if ( write_sgf_output ) writeSgf(out_stream, complete_output_file_base_name);
    else if ( write_ord_output ) writeOrd(out_stream);
    fclose(out_stream);
}

Nodeptr makeNumberedNode(int id, int layer, int position) {
#ifdef DEBUG
    printf("-> makeNumberedNode: id = %d, layer = %d, position = %d\n",
           id, layer, position);
#endif
    static int index = 0;
    Nodeptr new_node = (Nodeptr) calloc(1, sizeof(struct node_struct));
    new_node->name = calloc(strlen(nameFromId(id)) + 1, sizeof(char));
    strcpy(new_node->name, nameFromId(id));
    new_node->id = id;
    new_node->layer = layer;
    new_node->horizontal_position = position;
    // layer_index is set when node is inserted into layer
    new_node->layer_index = -1;
    new_node->up_edges = new_node->down_edges = NULL;
    new_node->up_degree = new_node->down_degree = 0;
    new_node->up_crossings = new_node->down_crossings = 0;
    new_node->marked = new_node->fixed = false;
    new_node->preorder_number = -1;
    master_node_list[index++] = new_node;
#ifdef DEBUG
    printf("<- makeNumberedNode, number_of_nodes = %d\n", number_of_nodes);
#endif
    return new_node;
}

void allocateNodeListsForLayers(void) {
    for ( int layer_num = 0; layer_num < number_of_layers; layer_num++ ) {
        Layerptr layer = layers[layer_num];
        layer->nodes = (Nodeptr *) calloc(layer->number_of_nodes, sizeof(Nodeptr));
    }
}

/**
 * inserts the node into the layer based on its position (for sgf)
 * checks for dupicate positions
 */
void insertIntoLayer(Nodeptr node, int layer_num, int num_nodes_so_far) {
#ifdef DEBUG
  fprintf(stderr, "-> insertIntoLayer, nodes_so_far = %d\n", num_nodes_so_far);
  writeNode(stderr, node);
#endif
    Layerptr layer = layers[layer_num];
    int current_index = num_nodes_so_far;
    while ( current_index > 0
            && layer->nodes[current_index - 1]->horizontal_position
                  >= node->horizontal_position ) {
      if ( layer->nodes[current_index - 1]->horizontal_position
            == node->horizontal_position ) {
        fprintf(stderr, "*** FATAL: two nodes have the same position on their layer\n");
        fprintf(stderr, "    nodes are [id,layer,position]: [%d,%d,%d] and [%d,%d,%d]\n",
                       layer->nodes[current_index-1]->id,
                       layer->nodes[current_index-1]->layer,
                       layer->nodes[current_index-1]->horizontal_position,
                       node->id, node->layer, node->horizontal_position);
        abort();
      }
      layer->nodes[current_index] = layer->nodes[current_index - 1];
      layer->nodes[current_index]->layer_index = current_index;
      current_index--;
    }
    layer->nodes[current_index] = node;
    node->layer_index = current_index;  
#ifdef DEBUG
  fprintf(stderr, "<- insertIntoLayer\n");
  writeNode(stderr, node);
#endif
}

/**
 * traverses master node list twice, once to compute the number of
 * nodes on each layer for allocation purposes and a second time to
 * add each node to its layer
 * Important: layers are kept sorted by position.
 * @todo might be better to use the same approach as with edges and
 * adjacency lists, i.e., use realloc and put the nodes on the layers
 * as we make them rather than using a separate function
 */
void addNodesToLayers(void) {
    for ( int index = 0; index < number_of_nodes; index++ ) {
        int layer_num = master_node_list[index]->layer;
        layers[layer_num]->number_of_nodes++;
        if ( layers[layer_num]->number_of_nodes > max_layer_width ) {
          max_layer_width = layers[layer_num]->number_of_nodes;
          max_width_layer = layer_num;
        }
    }
    allocateNodeListsForLayers();
    // number of nodes added so far
    int * current_num_nodes = calloc(number_of_layers, sizeof(int));
    for ( int layer_num = 0; layer_num < number_of_layers; layer_num++ ) {
        current_num_nodes[layer_num] = 0;
    }
    for ( int index = 0; index < number_of_nodes; index++ ) {
        Nodeptr node = master_node_list[index];
        int layer_num = node->layer;
        insertIntoLayer(node, layer_num, current_num_nodes[layer_num]);
        current_num_nodes[layer_num]++;
    }
    free(current_num_nodes);
}

void allocateLayers(void) {
    for ( int layer_num = 0; layer_num < number_of_layers; layer_num++ ) {
        layers[layer_num] = (Layerptr) calloc(1, sizeof(struct layer_struct));
        layers[layer_num]->number_of_nodes = 0;
        layers[layer_num]->nodes = NULL;
        layers[layer_num]->fixed = false;
    }
}

// The input algorithm (for dot and ord files) is as follows:
//   1. Read the ord file (first pass) and             - allocateLayersFromOrdFile()
//       (a) create each layer and expand the 'layers' array as needed
//       (b) count the number of nodes on each layer and store in
//           'number_of_nodes'; also count the global number of nodes
//       (c) allocate the 'nodes' array for each layer
//   2. Read the ord file again and                    - assignNodesToLayers()
//       (a) create each node
//       (b) add each node to the appropriate layer
//   3. Read the dot file (first pass) and             - allocateAdjacencyLists()
//       (a) count the 'up_degree' and 'down_degree' of each node
//       (b) go through all the nodes and allocate the 'up_edges' and
//           the 'down_edges'
//       (c) reset 'up_degree' and 'down_degree' to 0 so that edges can
//           be put in the right positions on the second pass 
//   4. Read the dot file again and put the nodes into the adjacency lists
//      based on the edges                             - createEdges()
//
// Note: The last phase ignores directions of the edges in the dot file and
// only looks at layer information to determine 'up' and 'down' edges for
// each node. For example, if a->b in the dot file and a is on layer 1 while
// b is on layer 0, then the edge is an up-edge for b and a down-edge for a.

/**
 * Creates a new node and maps its name to (a pointer to) its record
 * @param name the name of the node
 * @return (a pointer to) the newly created node
 */
Nodeptr makeNode( const char * name );

/**
 * Put a node in the next available position on a given layer
 */
static void addNodeToLayer( Nodeptr node, int layer );

/**
 * Creates a new layer with the next number; layers are created in
 * numerical sequence (used in first pass of reading .ord file)
 */
void makeLayer();

Nodeptr makeNode( const char * name )
{
  static int current_id = 0;

  Nodeptr new_node = (Nodeptr) calloc(1, sizeof(struct node_struct));
  new_node->name = (char *) calloc(strlen(name) + 1, sizeof(char));
  strcpy(new_node->name, name);
  // delay assignment of id's until edges are added so that the numbering
  // depends on .dot file only (easier to standardize)
  new_node->id = current_id++;
  new_node->layer = new_node->layer_index = -1; /* to indicate "uninitialized" */
  new_node->up_degree = new_node->down_degree = 0;
  new_node->up_edges = new_node->down_edges = NULL;
  new_node->up_crossings = new_node->down_crossings = 0;
  new_node->marked = new_node->fixed = false;
  new_node->preorder_number = -1;
  insertInHashTable(name, new_node);
  master_node_list[ new_node->id ] = new_node;
  return new_node;
}

void addNodeToLayer( Nodeptr node, int layer )
{
  static int current_layer = 0;
  static int current_index = 0;
  if ( layer != current_layer ) {
      current_layer = layer;
      current_index = 0;
  }
  node->layer = current_layer;
  node->layer_index = current_index;
  // need this when reading an ord file
  node->horizontal_position = current_index;
  layers[ layer ]->nodes[ current_index++ ] = node;
}

void makeLayer() {
    Layerptr new_layer = (Layerptr) calloc(1, sizeof(struct layer_struct));
    new_layer->number_of_nodes = 0;
    new_layer->nodes = NULL;
    if( number_of_layers >= layer_capacity ) {
        layer_capacity *= 2;
        layers
            = (Layerptr *) realloc(layers, layer_capacity * sizeof(Layerptr));
    }
    layers[number_of_layers++] = new_layer;
}

static void deallocateLayer(int layer_number) {
    free(layers[layer_number]->nodes);
    free(layers[layer_number]);
}

static void deallocateLayers(void) {
    for ( int i = 0; i < number_of_layers; i++ ) {
        deallocateLayer(i);
    }
    free(layers);
}

void addEdge(const char * source, const char * target)
{
#ifdef DEBUG
    printf("-> addEdge: %s, %s\n", source, target);
#endif
  static int num_edges_so_far = 0;
  Nodeptr node1 = getFromHashTable(source);
  if ( node1 == NULL ) {
    fprintf( stderr, "*** FATAL: source node %s does not exist.\n", source );
    abort();
  }
  Nodeptr node2 = getFromHashTable(target);
  if ( node2 == NULL ) {
    fprintf( stderr, "*** FATAL: target node %s does not exist.\n", target );
    abort();
  }
#ifdef DEBUG
  fprintf(stderr, " node1.position = %d, node2.position = %d\n",
          node1->layer_index, node2->layer_index);
#endif
  if ( node1->layer == node2->layer ) {
    fprintf( stderr, "*** FATAL: addEdge, nodes on same layer.\n" );
    fprintf( stderr, " Nodes %s and %s are on layer %d.\n",
             node1->name, node2->name, node1->layer);
    abort();
  }

  Nodeptr upper_node
    = ( node1->layer > node2->layer ) ? node1 : node2;
  Nodeptr lower_node
    = ( node1->layer < node2->layer ) ? node1 : node2;
  if ( upper_node->layer - lower_node->layer != 1 ) {
      fprintf( stderr, "*** FATAL: addEdge, nodes not on adjacent layers.\n" );
      fprintf( stderr, " Nodes %s is on layer %d and %s is on layer %d.\n",
               upper_node->name, upper_node->layer,
               lower_node->name, lower_node->layer);
      abort();
  }
  Edgeptr new_edge = calloc(1, sizeof(struct edge_struct));
  new_edge->up_node = upper_node;
  new_edge->down_node = lower_node;
  new_edge->crossings = 0;
  new_edge->fixed = false;
  // these arrays will not be allocated when addEdge() is called while
  // reading an sgf file; we need to be careful to fill them later
  /**
   * @todo this is awkward; would be much better to split this long
   * function into three pieces: check for errors, allocate the edge,
   * add to lists and/or update degrees
   */
  upper_node->down_edges
      = realloc(upper_node->down_edges,
                (upper_node->down_degree + 1) * sizeof(Edgeptr));
  lower_node->up_edges
      = realloc(lower_node->up_edges,
                (lower_node->up_degree + 1) * sizeof(Edgeptr));
  upper_node->down_edges[upper_node->down_degree++] = new_edge;
  lower_node->up_edges[lower_node->up_degree++] = new_edge;
  master_edge_list[num_edges_so_far++] = new_edge;
#ifdef DEBUG
    printf("<- addEdge: %s, %s\n", source, target);
#endif
}

/**
 * Sets number of nodes for the layer and allocates space for them.
 * Used only when reading dot and ord files.
 */
static void setNumberOfNodes( int layer, int number )
{
    layers[layer]->number_of_nodes = number;
    layers[layer]->nodes = (Nodeptr *) calloc(number, sizeof(Nodeptr));
}

/**
 * Implements the first pass of reading the ord file: allocates a record for
 * each layer and space for the nodes on each layer. Creates the nodes
 * and maps their names to (pointers to) their records. Counts the total
 * number of nodes.
 */
static void allocateLayersFromOrdFile( const char * ord_file )
{
  FILE * in = fopen( ord_file, "r" );
  if( in == NULL )
    {
      fprintf( stderr, "*** FATAL ERROR: Unable to open file %s for input\n", ord_file );
      exit( EXIT_FAILURE );
    }
  layer_capacity = MIN_LAYER_CAPACITY;
  layers = (Layerptr *) calloc( layer_capacity, sizeof(Layerptr) );

  int layer;
  int expected_layer = 0;
  char name_buf[MAX_NAME_LENGTH];
  while ( nextLayer( in, & layer ) )
    {
      if( layer != expected_layer )
        {
          fprintf( stderr, "*** FATAL error: Expected layer %d, found layer %d\n",
                   expected_layer, layer );
          abort();
        }
      expected_layer++;
      makeLayer();
      int node_count = 0;
      while ( nextNode( in, name_buf ) )
        {
          node_count++;
          number_of_nodes++;    /* global node count */
        }
      setNumberOfNodes( layer, node_count );
      if ( node_count > max_layer_width ) max_layer_width = node_count;
  }
  fclose( in );
}

/**
 * Reads the ord file and put the nodes on their appropriate layers
 */
static void assignNodesToLayers( const char * ord_file )
{
  FILE * in = fopen( ord_file, "r" );
  if ( in == NULL ) {
    fprintf(stderr, "*** FATAL ERROR: file %s could not be opened\n", ord_file);
    exit(EXIT_FAILURE);
  }

  int layer;
  char name_buf[MAX_NAME_LENGTH];
  while( nextLayer( in, & layer ) )
    {
      while( nextNode( in, name_buf ) )
        {
          Nodeptr node = makeNode( name_buf );
          addNodeToLayer( node, layer );
        }
    }
  fclose( in );
}

/**
 * Increments the degrees of the endpoints of an edge between source and target
 */
void incrementDegrees( const char * source, const char * target )
{
  Nodeptr node1 = getFromHashTable( source );
  if( node1 == NULL )
    {
      fprintf( stderr, "*** FATAL error: Node '%s' does not exist in .ord file\n"
               " edge is %s->%s\n", source, source, target);
      abort();
    }
  Nodeptr node2 = getFromHashTable( target );
  if( node2 == NULL )
    {
      fprintf( stderr, "*** FATAL error: Node '%s' does not exist in .ord file\n"
               " edge is %s->%s\n", target, source, target);
      abort();
    }
  Nodeptr upper_node
    = ( node1->layer > node2->layer ) ? node1 : node2;
  Nodeptr lower_node
    = ( node1->layer < node2->layer ) ? node1 : node2;
  upper_node->down_degree++;
  lower_node->up_degree++;
}

/**
 * Gets any comments occuring right after the graph name from the dot
 * file that was recently initialized using initDot() and stores them
 * as comments of the graph
 */
void getCommentsFromDotFile(void) {
    char buffer[MAX_NAME_LENGTH];
    startGettingDotComments();
    startAddingComments();
    while ( getNextDotComment(buffer) ) {
        addComment(buffer, true);
    }
}


/**
 * Reads the dot file and makes room for nodes on all the adjacency lists;
 * resets up and down node degrees. This is the first pass of reading the dot
 * file.  Also saves the name of the graph and any comments occuring
 * right after the name.
 */
void allocateAdjacencyLists( const char * dot_file )
{
  FILE * in = fopen( dot_file, "r" );
  if( in == NULL )
    {
      fprintf( stderr, "*** FATAL ERROR: Unable to open file %s for input\n", dot_file );
      exit( EXIT_FAILURE );
    }
  initDot(in);
  getNameFromDotFile(graph_name);
  getCommentsFromDotFile();
  // read the edges and use each edge to update the appropriate degree for
  // each endpoint
  char src_buf[MAX_NAME_LENGTH];
  char dst_buf[MAX_NAME_LENGTH];
  while ( nextEdge( in, src_buf, dst_buf ) )
    {
#ifdef DEBUG
      printf( " new edge: %s -> %s\n", src_buf, dst_buf );
#endif
      number_of_edges++;
      incrementDegrees( src_buf, dst_buf );
    }
  // allocate adjacency lists for all nodes based on the appropriate degrees
  int layer = 0;
  for( ; layer < number_of_layers; layer++ )
    {
      int position = 0;
      for( ; position < layers[ layer ]->number_of_nodes; position++ )
        {
          Nodeptr node = layers[ layer ]->nodes[ position ];
          node->up_edges
            = (Edgeptr *) calloc( node->up_degree, sizeof(Nodeptr) );
          node->down_edges
            = (Edgeptr *) calloc( node->down_degree, sizeof(Nodeptr) );
          node->up_degree = 0;
          node->down_degree = 0;
        }
    }
  fclose( in );
}

/**
 * Reads the dot file and adds all the edges. This is the second pass.
 */
void createEdges( const char * dot_file )
{
  FILE * in = fopen( dot_file, "r" );
  if( in == NULL )
    {
      fprintf( stderr, "*** FATAL ERROR: Unable to open file %s for input\n", dot_file );
      exit( EXIT_FAILURE );
    }
  initDot( in );
  char src_buf[MAX_NAME_LENGTH];
  char dst_buf[MAX_NAME_LENGTH];
  while ( nextEdge( in, src_buf, dst_buf ) )
    {
      addEdge( src_buf, dst_buf );
    }
  fclose( in );
}

/**
 * @return number of nodes whose up_degree and
 * down_degree are both zero
 */
int countIsolatedNodes()
{
  /**
   * @todo
   * Should eliminate the isolated nodes altogether; on each layer -
   * <pre>
   *  deallocate isolated nodes and mark positions with NULL
   *  position = real_position = 0;
   *  for position < number_of_nodes, position++
   *     while real_position < number_of_nodes and node[position] == NULL
   *        real_position++
   *     if node[position] == NULL && real_position < number_of_nodes
   *        node[position] = node[real_position++]
   * </pre>
   */
  int isolated_nodes = 0;
  int layer = 0;
  for( ; layer < number_of_layers; layer++ )
    {
      int position = 0;
      for( ; position < layers[ layer ]->number_of_nodes; position++ )
        {
          Nodeptr node = layers[ layer ]->nodes[ position ];
          if( node->up_degree + node->down_degree == 0 )
            isolated_nodes++;
        }
    }
  return isolated_nodes;
}

void readDotAndOrd( const char * dot_file, const char * ord_file )
{
  number_of_nodes = 0;
  number_of_edges = 0;
  number_of_layers = 0;
  startAddingComments();
  allocateLayersFromOrdFile( ord_file );
  master_node_list = (Nodeptr *) calloc( number_of_nodes, sizeof(Nodeptr) );
  initHashTable( number_of_nodes );
  assignNodesToLayers( ord_file );
#ifdef DEBUG
  printf( "Master node list after reading ord file:\n" );
  for ( int i = 0; i < number_of_nodes; i++ ) {
    printf( "%s, layer = %d, position = %d\n", master_node_list[i]->name,
            master_node_list[i]->layer, master_node_list[i]->layer_index );
  }
#endif
  allocateAdjacencyLists( dot_file );
  // at this point the number of edges is known
  master_edge_list = (Edgeptr *) calloc( number_of_edges, sizeof(Edgeptr) );
  createEdges( dot_file );
  number_of_isolated_nodes = countIsolatedNodes();
  removeHashTable();
}

/********* End - dot and ord input *************/

// --------------- Handling of comments

void startAddingComments(void) {
    comments = calloc(1, sizeof(char));
    comments[0] = '\0';         /* so strlen will be 0 */
}

void addComment(const char * comment, bool needs_eol) {
    // one extra space for the '\n', another for '\0'
    comments = realloc(comments, strlen(comments) + strlen(comment) + 2);
    strcat(comments, comment);
    if ( needs_eol )
        strcat(comments, "\n");
}

/**
 * pointer to start of next comment
 */
static char * next_comment;

void startGettingComments(void) {
    next_comment = comments;
}

char * getNextComment(char * comment_buffer) {
    char * end_comment = strchr(next_comment, '\n');
    if ( end_comment == NULL ) return NULL;
    while ( next_comment != end_comment ) {
        *comment_buffer = *next_comment;
        comment_buffer++;
        next_comment++;
    }
    *comment_buffer = '\0';
    return ++next_comment;
}

// --------------- *** Deallocation *** -------------

static void deallocateNodes(void) {
    for ( int i = 0; i < number_of_nodes; i++ ) {
        Nodeptr node = master_node_list[i];
        free(node->name);
        free(node->up_edges);
        free(node->down_edges);
    }
    free(master_node_list);
}

static void deallocateEdges(void) {
    for ( int i = 0; i < number_of_edges; i++ ) {
        Edgeptr edge = master_edge_list[i];
        free(edge);
    }
    free(master_edge_list);
}

static void deallocateComments(void) {
    free(comments);
}

void deallocateGraph(void) {
    deallocateNodes();
    deallocateEdges();
    deallocateLayers();
    deallocateComments();
}

// --------------- Output to dot and ord files

static void writeNodes( FILE * out, Layerptr layerptr )
{
  int i = 0;
  for( ; i < layerptr->number_of_nodes; i++ )
    {
      outputNode( out, layerptr->nodes[i]->name );
    }
}

void writeOrd(FILE * out)
{
  ordPreamble( out, graph_name, "" );
  int layer = 0;
  for( ; layer < number_of_layers; layer++ )
    {
      beginLayer( out, layer, "" );
      writeNodes( out, layers[ layer ] );
      endLayer( out );
    }
  fclose( out );
}

void writeDot( const char * dot_file_name,
               const char * graph_name,
               const char * header_information,
               const Edgeptr * edge_list,
               int edge_list_length
               )
{
  FILE * out = fopen( dot_file_name, "w" );
  if( out == NULL )
    {
      fprintf( stderr, "*** FATAL ERROR: Unable to open file %s for output\n", dot_file_name );
      exit( EXIT_FAILURE );
    }
  dotPreamble( out, graph_name, header_information );
  for ( int i = 0; i < edge_list_length; i++ )
    {
      Edgeptr current = edge_list[i];
      Nodeptr up_node = current->up_node;
      Nodeptr down_node = current->down_node;
      outputEdge( out, down_node->name, up_node->name );
    }
  endDot( out );
  fclose( out );
}

// --------------- Debugging output --------------

void writeNode(FILE * outstream, Nodeptr node) {
  if ( node == NULL ) {
    fprintf(outstream, "(null)\n");
    return;
  }
  fprintf(outstream, "  [%d] layer = %d index = %d position = %d"
                     " up = %d down = %d up_x = %d down_x = %d\n",
         node->id, node->layer, node->layer_index, node->horizontal_position,
         node->up_degree, node->down_degree,
         node->up_crossings, node->down_crossings);
  fprintf(outstream, "      ^^^^ up");
  int i = 0;
  for( ; i < node->up_degree; i++ ) {
      Edgeptr edge = node->up_edges[i];
      fprintf(outstream, " %s", edge->up_node->name );
    }
  fprintf(outstream, "\n");
  fprintf(outstream, "      ___ down");
  i = 0;
  for( ; i < node->down_degree; i++ ) {
      Edgeptr edge = node->down_edges[i];
      fprintf(outstream, " %s", edge->down_node->name );
    }
  fprintf(outstream, "\n");
}

void writeEdge(FILE * outstream, Edgeptr edge) {
  if ( edge == NULL ) {
    fprintf(outstream, "(null)\n");
    return;
  }
  fprintf(outstream, " -- edge: %s, %s\n", edge->down_node->name, edge->up_node->name);
  fprintf(outstream, "   xings = %d, nv = %d, fixed = %d\n",
          edge->crossings, edge->nonverticality, edge->fixed);
}

void writeLayer(FILE * outstream, int layer) {
  fprintf(outstream, "  --- layer %d nodes = %d fixed = %d\n",
         layer, layers[layer]->number_of_nodes, layers[layer]->fixed );
  int node = 0;
  for( ; node < layers[layer]->number_of_nodes; node++ ) {
    writeNode(outstream, layers[layer]->nodes[node]);
  }
}

void writeGraph(FILE * outstream) {
  fprintf(outstream, "+++ begin-graph %s nodes = %d, edges = %d, layers = %d\n",
         graph_name, number_of_nodes, number_of_edges, number_of_layers);
  int layer = 0;
  for( ; layer < number_of_layers; layer++ ) {
      writeLayer(outstream, layer);
  }
  fprintf(outstream, " ---- edges ----\n");
  for ( int index = 0; index < number_of_edges; index++ ) {
      writeEdge(outstream, master_edge_list[index]);
  }
  fprintf(outstream, "=== end-graph\n");
}

#ifdef TEST

int main(int argc, char * argv[]) {
  FILE * instream = fopen(argv[1], "r");
  readSgf(instream);
  writeGraph(stdout);
  return EXIT_SUCCESS;
}

#endif
