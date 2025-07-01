/**
 * @file dot_and_ord_to_sgf.c
 * @brief Program to convert a dot and ord file to an equivalent sgf file
 * @author Matt Stallmann
 * @date 2011/06/16
 *
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
 *
 * edges are directed so that the nodes appearing earlier in the input are
 * sources
 */


#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<limits.h>
#include<assert.h>

#include"defs.h"
#include"graph_io.h"
#include"graph.h"

char * heuristic = NULL;
char * preprocessor = NULL;

/**
 * prints usage message
 */
static void printUsage( void ) {
  printf( "Usage: dot_and_ord_to_sgf DOT_FILE_NAME ORD_FILE_NAME\n" );
  printf( " reads files DOT_FILE_NAME and ORD_FILE_NAME to produce an sgf file\n" );
  printf( " printing it on standard output\n");
}

/**
 * Writes an sgf file based on the current graph to standard output
 */
static void write_sgf( void ) {
  startGettingComments();
  char buffer[MAX_NAME_LENGTH];
  while ( getNextComment(buffer) ) {
      printf("c %s\n", buffer);
  }
  printf( "t %s %d %d %d\n",
           graph_name,
           number_of_nodes,
           number_of_edges,
           number_of_layers
           );
  
  // add lines for the nodes
  for( int layer = 0; layer < number_of_layers; layer++ ) {
    for( int position = 0;
         position < layers[ layer ]->number_of_nodes;
         position++ ) {
      Nodeptr node = layers[ layer ]->nodes[ position ];
      printf( "n %d %d %d\n", node->id, layer, position );
    }
  }

  // add lines for the edges
  for( int index = 0; index < number_of_edges; index++ ) {
      Edgeptr edge = master_edge_list[index];
      printf( "e %d %d\n", edge->down_node->id, edge->up_node->id );
  }
}

int main( int argc, char * argv[] )
{
  if( argc != 3 ) {
      printUsage();
      return EXIT_FAILURE;
  }
  const char * dot_file_name = argv[1];
  const char * ord_file_name = argv[2];

  readDotAndOrd( dot_file_name, ord_file_name );
  write_sgf();

  return EXIT_SUCCESS;
}
