/**
 * @file graph_io.c
 * @brief Implementation of functions that create graph structures from input
 * .dot and .ord files. 
 * @author Matt Stallmann, based on Saurabh Gupta's crossing heuristics
 * implementation.
 * @date 2008/12/19
 * $Id: graph_io.c 97 2014-09-10 17:05:19Z mfms $
 */

#include"graph.h"
#include"hash.h"
#include"defs.h"
#include"dot.h"
#include"ord.h"
#include"sgf.h"

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<assert.h>

#define MIN_LAYER_CAPACITY 1 

Nodeptr * master_node_list;
Edgeptr * master_edge_list;
int number_of_nodes = 0;
int number_of_layers = 0;
int number_of_edges = 0;
int number_of_isolated_nodes = 0;
Layerptr * layers = NULL;
char graph_name[MAX_NAME_LENGTH];

// for debugging

void printNode(Nodeptr node);
void printEdge(Edgeptr edge);
void printLayer(int layer);
void printGraph();

// initial allocated size of layer array (will double as needed)
static int layer_capacity = MIN_LAYER_CAPACITY;

int countIsolatedNodes();

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
void addEdge(const char * name1, const char * name2);

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

static char name_buffer[MAX_NAME_LENGTH];

/**
 * utility function that converts a number to a string
 * @return a pointer to the buffer - have to use strcpy to use the
 * string since buffer contents will change
 */
static char * nameFromId(int id) {
    sprintf(name_buffer, "%d", id);
    return name_buffer;
}

/**
 * assumes master_node_list has been allocated to accommodate number
 * of nodes in header (sgf)
 */
void addToNodeList(Nodeptr node) {
    static int index = 0;
    master_node_list[index++] = node;
}

/**
 * Creates a new node with the given id number
 *   and performs 2. (a)-(e) above
 * @param id the id number of the node
 * @param layer the layer of the node
 * @param position the position of the node on its layer
 * @return (a pointer to) the newly created node
 */
Nodeptr makeNumberedNode(int id, int layer, int position) {
#ifdef DEBUG
    printf("-> makeNumberedNode: id = %d, layer = %d, position = %d\n",
           id, layer, position);
#endif
    Nodeptr new_node = (Nodeptr) malloc(sizeof(struct node_struct));
    char * name = malloc(strlen(nameFromId(id)) + 1);
    strcpy(name, nameFromId(id));
    new_node->name = name;
    new_node->id = id;
    new_node->layer = layer;
    new_node->position = position;
    new_node->up_edges = new_node->down_edges = NULL;
    new_node->up_degree = new_node->down_degree = 0;
    new_node->up_crossings = new_node->down_crossings = 0;
    new_node->marked = new_node->fixed = false;
    new_node->preorder_number = -1;
    insertInHashTable(new_node->name, new_node);
    addToNodeList(new_node);
    return new_node;
}

/**
 * Creates the struct for each node using makeNumberedNode() and adds
 * (a pointer to) it to the master list
 */
void readSgfNodes(void) {
    while ( getNextNode() ) {
        makeNumberedNode(sgf_node.id, sgf_node.layer, sgf_node.position);
    }
}

/**
 * Creates the struct for each edge and adds (a pointer to) it to the
 * master list; uses addEdge to do sanity checks and retrieve node
 * pointers
 */
void readSgfEdges(void) {
    while ( getNextEdge() ) {
        char source_name[MAX_NAME_LENGTH];
        strcpy(source_name, nameFromId(sgf_edge.source));
        char target_name[MAX_NAME_LENGTH];
        strcpy(target_name, nameFromId(sgf_edge.target));
        addEdge(source_name, target_name);
    }
}

void allocateNodeListsForLayers(void) {
    for ( int layer_num = 0; layer_num < number_of_layers; layer_num++ ) {
        Layerptr layer = layers[layer_num];
        layer->nodes = (Nodeptr *) calloc(layer->number_of_nodes, sizeof(Nodeptr));
    }
}

/**
 * traverses master node list twice, once to compute the number of
 * nodes on each layer for allocation purposes and a second time to
 * add each node to its layer
 * @todo might be better to use the same approach as with edges and
 * adjacency lists, i.e., use realloc and put the nodes on the layers
 * as we make them rather than using a separate function
 */
void addNodesToLayers(void) {
    for ( int index = 0; index < number_of_nodes; index++ ) {
        int layer_num = master_node_list[index]->layer;
        layers[layer_num]->number_of_nodes++;
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
        layers[layer_num]->nodes[current_num_nodes[layer_num]++] = node;
    }
    free(current_num_nodes);
}

/**
 * creates a layer struct for each layer, assuming array 'layers' is allocated
 */
void allocateLayers(void) {
    for ( int layer_num = 0; layer_num < number_of_layers; layer_num++ ) {
        layers[layer_num] = (Layerptr) malloc(sizeof(struct layer_struct));
        layers[layer_num]->number_of_nodes = 0;
        layers[layer_num]->nodes = NULL;
        layers[layer_num]->fixed = false;
    }
}

void readSgf(FILE * sgf_stream) {
    initSgf(sgf_stream);
    getNameFromSgfFile(graph_name);
    number_of_nodes = getNumberOfNodes();
    number_of_edges = getNumberOfEdges();
    number_of_layers = getNumberOfLayers();
    initHashTable(number_of_nodes);
    master_node_list = (Nodeptr *) calloc(number_of_nodes, sizeof(Nodeptr));
    master_edge_list = (Edgeptr *) calloc(number_of_edges, sizeof(Edgeptr));
    layers = (Layerptr *) calloc(number_of_layers, sizeof(Layerptr));
    allocateLayers();
    readSgfNodes();
    readSgfEdges();
    allocateNodeListsForLayers();
    addNodesToLayers();
    number_of_isolated_nodes = countIsolatedNodes();
    removeHashTable();
}


// The input algorithm (for dot and ord files) is as follows:
//   1. Read the ord file (first pass) and
//       (a) create each layer and expand the 'layers' array as needed
//       (b) count the number of nodes on each layer and store in
//           'number_of_nodes'; also count the global number of nodes
//       (c) allocate the 'nodes' array for each layer
//   2. Read the ord file again and 
//       (a) create each node
//       (b) add each node to the appropriate layer
//   3. Read the dot file (first pass) and
//       (a) count the 'up_degree' and 'down_degree' of each node
//       (b) go through all the nodes and allocate the 'up_edges' and
//           the 'down_edges'
//       (c) reset 'up_degree' and 'down_degree' to 0 so that edges can
//           be put in the right positions on the second pass 
//   4. Read the dot file again and put the nodes into the adjacency lists
//      based on the edges
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
void addNodeToLayer( Nodeptr node, int layer );

/**
 * Creates a new layer with the next number; layers are created in
 * numerical sequence (used in first pass of reading .ord file)
 */
void makeLayer();

Nodeptr makeNode( const char * name )
{
  static int current_id = 0;

  Nodeptr new_node = (Nodeptr) malloc( sizeof(struct node_struct));
  new_node->name = (char *) malloc( strlen(name) + 1 );
  strcpy( new_node->name, name );
  // delay assignment of id's until edges are added so that the numbering
  // depends on .dot file only (easier to standardize)
  new_node->id = current_id++;
  new_node->layer = new_node->position = -1; /* to indicate "uninitialized" */
  new_node->up_degree = new_node->down_degree = 0;
  new_node->up_edges = new_node->down_edges = NULL;
  new_node->up_crossings = new_node->down_crossings = 0;
  new_node->marked = new_node->fixed = false;
  new_node->preorder_number = -1;
  insertInHashTable( name, new_node );
  master_node_list[ new_node->id ] = new_node;
  return new_node;
}

void addNodeToLayer( Nodeptr node, int layer )
{
  static int current_layer = 0;
  static int current_position = 0;
  if( layer != current_layer )
    {
      current_layer = layer;
      current_position = 0;
    }
  node->layer = current_layer;
  node->position = current_position;
  layers[ layer ]->nodes[ current_position++ ] = node;
}

void makeLayer()
{
  Layerptr new_layer = (Layerptr) malloc( sizeof(struct layer_struct) );
  new_layer->number_of_nodes = 0;
  new_layer->nodes = NULL;
  if( number_of_layers >= layer_capacity )
    {
      layer_capacity *= 2;
      layers
        = (Layerptr *) realloc( layers, layer_capacity * sizeof(Layerptr) );
    }
  layers[ number_of_layers++ ] = new_layer;
}

void addEdge(const char * name1, const char * name2)
{
#ifdef DEBUG
    printf("-> addEdge: %s, %s\n", name1, name2);
#endif
  static int num_edges_so_far = 0;
  Nodeptr node1 = getFromHashTable(name1);
  Nodeptr node2 = getFromHashTable(name2);
  if ( node1->layer == node2->layer ) {
    fprintf( stderr, "FATAL: addEdge, nodes on same layer.\n" );
    fprintf( stderr, " Nodes %s and %s are on layer %d.\n",
             node1->name, node2->name, node1->layer);
    abort();
  }

  if ( node1 == NULL ) {
      fprintf(stderr, "FATAL: addEdge, missing node %s\n", node1->name);
      abort();
  }

  if ( node2 == NULL ) {
      fprintf(stderr, "FATAL: addEdge, missing node %s\n", node2->name);
      abort();
  }

  Nodeptr upper_node
    = ( node1->layer > node2->layer ) ? node1 : node2;
  Nodeptr lower_node
    = ( node1->layer < node2->layer ) ? node1 : node2;
  if ( upper_node->layer - lower_node->layer != 1 ) {
      fprintf( stderr, "FATAL: addEdge, nodes not on adjacent layers.\n" );
      fprintf( stderr, " Nodes %s is on layer %d and %s is on layer %d.\n",
               upper_node->name, upper_node->layer,
               lower_node->name, lower_node->layer);
      abort();
  }
  Edgeptr new_edge = malloc( sizeof(struct edge_struct) );
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
    printf("<- addEdge: %s, %s\n", name1, name2);
#endif
}

/**
 * Sets number of nodes for the layer and allocates space for them. Note: the
 * global number of nodes is updated in allocateLayers().
 */
static void setNumberOfNodes( int layer, int number )
{
  layers[ layer ]->number_of_nodes = number;
  layers[ layer ]->nodes = (Nodeptr *) calloc( number, sizeof(Nodeptr) );
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
      fprintf( stderr, "Unable to open file %s for input\n", ord_file );
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
          fprintf( stderr, "Fatal error: Expected layer %d, found layer %d\n",
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
  }
  fclose( in );
}

/**
 * Reads the ord file and put the nodes on their appropriate layers
 */
static void assignNodesToLayers( const char * ord_file )
{
  FILE * in = fopen( ord_file, "r" );
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
 * Increments the degrees of the endpoints of an edge between name1 and name2
 */
void incrementDegrees( const char * name1, const char * name2 )
{
  Nodeptr node1 = getFromHashTable( name1 );
  if( node1 == NULL )
    {
      fprintf( stderr, "Fatal error: Node '%s' does not exist in .ord file\n"
               " edge is %s->%s\n", name1, name1, name2);
      abort();
    }
  Nodeptr node2 = getFromHashTable( name2 );
  if( node2 == NULL )
    {
      fprintf( stderr, "Fatal error: Node '%s' does not exist in .ord file\n"
               " edge is %s->%s\n", name2, name1, name2);
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
 * Reads the dot file and makes room for nodes on all the adjacency lists;
 * resets up and down node degrees. This is the first pass of reading the dot
 * file.  Also saves the name of the graph.
 */
void allocateAdjacencyLists( const char * dot_file )
{
  FILE * in = fopen( dot_file, "r" );
  if( in == NULL )
    {
      fprintf( stderr, "Unable to open file %s for input\n", dot_file );
      exit( EXIT_FAILURE );
    }
  initDot( in );
  getNameFromDotFile( graph_name );
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
  allocateLayersFromOrdFile( ord_file );
  master_node_list = (Nodeptr *) calloc( number_of_nodes, sizeof(Nodeptr) );
  initHashTable( number_of_nodes );
  assignNodesToLayers( ord_file );
#ifdef DEBUG
  printf( "Master node list after reading ord file:\n" );
  for ( int i = 0; i < number_of_nodes; i++ ) {
    printf( "%s, layer = %d, position = %d\n", master_node_list[i]->name,
            master_node_list[i]->layer, master_node_list[i]->position );
  }
#endif
  allocateAdjacencyLists( dot_file );
  // at this point the number of edges is known
  master_edge_list = (Edgeptr *) calloc( number_of_edges, sizeof(Edgeptr) );
  createEdges( dot_file );
  number_of_isolated_nodes = countIsolatedNodes();
  removeHashTable();
}

// --------------- Handling of comments

void startAddingComments(void) {
    comments = calloc(1, sizeof(char));
    comments[0] = '\0';         /* so strlen will be 0 */
}

void addComment(const char * comment) {
    comments = realloc(comments, strlen(comments) + strlen(comment) + 2);
    strcat(comments, comment);
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
    return ++next_comment;
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

/**
 * @todo ord file should have information about heuristic that created it, etc.,
 * embedded in it. Use of the file/graph name can be problematic if we want to
 * check what happens when one heuristic follows another for a whole class
 * using a script. There needs to be some way to differentiate among these
 * files.
 */
void writeOrd( const char * ord_file )
{
  FILE * out = fopen( ord_file, "w" );
  if( out == NULL )
    {
      fprintf( stderr, "Unable to open file %s for output\n", ord_file );
      exit( EXIT_FAILURE );
    }
  ordPreamble( out, graph_name, "" );
  int layer = 0;
  for( ; layer < number_of_layers; layer++ )
    {
      beginLayer( out, layer, "heuristic-based" );
      writeNodes( out, layers[ layer ] );
      endLayer( out );
    }
  fclose( out );
}

void writeDot( const char * dot_file_name,
               const char * graph_name,
               const char * header_information,
               Edgeptr * edge_list,
               int edge_list_length
               )
{
  FILE * out = fopen( dot_file_name, "w" );
  if( out == NULL )
    {
      fprintf( stderr, "Unable to open file %s for output\n", dot_file_name );
      exit( EXIT_FAILURE );
    }
  dotPreamble( out, graph_name, header_information );
  for ( int i = 0; i < edge_list_length; i++ )
    {
      Edgeptr current = edge_list[i];
      Nodeptr up_node = current->up_node;
      Nodeptr down_node = current->down_node;
      outputEdge( out, up_node->name, down_node->name );
    }
  endDot( out );
  fclose( out );
}

// --------------- Debugging output --------------

void printNode( Nodeptr node )
{
  printf("    [%3d ] %s layer=%d position=%d up=%d down=%d up_x=%d down_x=%d\n",
         node->id, node->name, node->layer, node->position,
         node->up_degree, node->down_degree,
         node->up_crossings, node->down_crossings );
  printf("      ^^^^up");
  int i = 0;
  for( ; i < node->up_degree; i++ )
    {
      Edgeptr edge = node->up_edges[i];
      printf(" %s", edge->up_node->name );
    }
  printf("\n");
  printf("      __down");
  i = 0;
  for( ; i < node->down_degree; i++ )
    {
      Edgeptr edge = node->down_edges[i];
      printf(" %s", edge->down_node->name );
    }
  printf("\n");
}

void printEdge(Edgeptr edge) {
    printf(" -- edge: %s, %s\n", edge->down_node->name, edge->up_node->name);
    printf("   crossings = %d, fixed = %d\n", edge->crossings, edge->fixed);
}

void printLayer( int layer )
{
  printf("  --- layer %d nodes=%d fixed=%d\n",
         layer, layers[layer]->number_of_nodes, layers[layer]->fixed );
  int node = 0;
  for( ; node < layers[layer]->number_of_nodes; node++ )
    {
      printNode( layers[layer]->nodes[node] );
    }
}

void printGraph()
{
  printf("+++ begin-graph %s nodes=%d, edges = %d, layers=%d\n",
         graph_name, number_of_nodes, number_of_edges, number_of_layers);
  int layer = 0;
  for( ; layer < number_of_layers; layer++ ) {
      printLayer( layer );
  }
  printf(" ---- edges ----\n");
  for ( int index = 0; index < number_of_edges; index++ ) {
      printEdge(master_edge_list[index]);
  }
  printf("=== end-graph\n");
}

#ifdef TEST

int main( int argc, char * argv[] )
{
  readDotAndOrd( argv[1], argv[2] );
  printGraph();
  fprintf( stderr, "Average number of probes = %5.2f\n",
           getAverageNumberOfProbes() );
  return 0;
}

#endif

/*  [Last modified: 2020 12 30 at 14:57:53 GMT] */
