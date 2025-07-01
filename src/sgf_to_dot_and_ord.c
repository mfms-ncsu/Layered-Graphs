/**
 * @file sgf_to_dot_and_ord.c
 * @brief Program to convert an sgf file to a equivalent dot and ord files
 * @author Matt Stallmann
 * @date 2023/06/29
 * @attention position information in the sgf file is not preserved,
 *            only the sequence of nodes on each layer
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
  printf("Usage: sgf_to_dot_and_ord SGF_FILE_NAME\n"
         " reads files SGF_FILE_NAME and produces dot and ord files with the same base name\n"
         " the dot and ord files will appear in the current directory\n");
}


int main(int argc, char * argv[]){
  if( argc != 2 ) {
      printUsage();
      return EXIT_FAILURE;
  }

  const char * sgf_file_name = argv[1];

  FILE * input_stream = fopen(sgf_file_name, "r");
  if ( input_stream == NULL ) {
    fprintf(stderr, "*** Error: File %s could not be opened ***", sgf_file_name);
    exit(EXIT_FAILURE);
  }

  readSgf(input_stream);
  fclose(input_stream);


  char base_name[MAX_NAME_LENGTH + 1];
  char dot_file_name[MAX_NAME_LENGTH + 1];
  char ord_file_name[MAX_NAME_LENGTH + 1];
  char header_info[MAX_NAME_LENGTH + 1];

  getBaseName(base_name, sgf_file_name);
  // leave room for extensions
  strncpy(dot_file_name, base_name, MAX_NAME_LENGTH - 4);
  strncpy(ord_file_name, base_name, MAX_NAME_LENGTH - 4);
  strncat(dot_file_name, ".dot", 4);
  strncat(ord_file_name, ".ord", 4);
  sprintf(header_info, "created by sgf_to_dot_and_ord: %d nodes, %d edges, %d layers\n",
          number_of_nodes, number_of_edges, number_of_layers);

  writeDot(dot_file_name, graph_name, header_info, master_edge_list, number_of_edges);
  
  FILE * out_stream = fopen(ord_file_name, "w");
  writeOrd(out_stream);
  fclose(out_stream);

  return EXIT_SUCCESS;
}
