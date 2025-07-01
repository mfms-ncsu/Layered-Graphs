/**
 * @file graph.h
 * @brief Definition of data structures and access functions for a layered
 * graph.
 * @author Matt Stallmann, based on Saurabh Gupta's crossing heuristics
 * implementation.
 * @date 2008/12/19
 * $Id: graph.h 90 2014-08-13 20:31:25Z mfms $
 *
 * Global data
 *  - number_of_layers
 *  - layers: an array of pointers to layer_struct's
 *  - graph_name: used for output
 * Layers are referred to by number except when internal info is needed.<br>
 * Nodes are referred to by pointers to node_struct's and all information
 * about a node (including layer and position) is stored in the struct.
 */

#include<stdbool.h>
#include<stdio.h>

#ifndef GRAPH_H
#define GRAPH_H

#include "constants.h"

typedef struct node_struct * Nodeptr;
typedef struct edge_struct * Edgeptr;
typedef struct layer_struct * Layerptr;

struct node_struct
{
  char * name;
  int id;                       /* unique identifier */
  int layer;
  /**
   * the index of the node in the array of nodes on the layer
   * @invariant this must be kept consistent when the layer is sorted or rearranged
   */
  int layer_index;
  /**
   * the actual position used to compute verticality
   * @invariant nodes on a layer are sorted by increasing horizontal_position
   */
  int horizontal_position;
  int up_degree;
  int down_degree;

  Edgeptr * up_edges;
  Edgeptr * down_edges;

  // for heuristics that require sorting -- in most cases this will be an int, but
  // barycenter involves fractions
  double weight;

  // for gbfs -- see 2001 JEA paper by Stallmann et al.
  int distance;
  int depth;

  // Added on 09-11-08 for max. crossings node heuristic
  bool fixed;
  int up_crossings;
  int down_crossings;
  
  // for DFS and GBFS
  bool marked;
  int preorder_number;
};

#define DEGREE( node ) ( node->up_degree + node->down_degree )
#define CROSSINGS( node ) ( node->up_crossings + node->down_crossings )

struct edge_struct {
  Nodeptr up_node;
  Nodeptr down_node;
  int crossings;
  int nonverticality;

  // for heuristics
  /**
   * true if edge has been processed in current iteration
   */
  bool fixed;
};

struct layer_struct {
  int number_of_nodes;
  Nodeptr * nodes;

  // for heuristics that fix layers during an iteration
  bool fixed;
};

// The following are defined in graph_io.c

/**
 * Contains all nodes of the graph; allows nodes to be accessed sequentially
 * in heuristics that require it; this list may be sorted or permuted
 * randomly
 */
extern Nodeptr * master_node_list;

/**
 * Contains all edges of the graph; allows edges to be accessed sequentially
 * in heuristics that require it; this list may be sorted or permuted
 * randomly
 */
extern Edgeptr * master_edge_list;

extern int number_of_layers;
extern int number_of_nodes;
extern int number_of_edges;
extern int number_of_isolated_nodes;
extern int max_layer_width;
extern int max_width_layer;
extern Layerptr * layers;
extern char graph_name[MAX_NAME_LENGTH];
extern char * comments;

/** print utilities for debugging, definitions in graph_io.c */
void printNode(FILE * stream, Nodeptr node);
void printEdge(FILE * stream, Edgeptr edge);

/**
 * @brief prints all nodes on a layer along with their adjacencies in a simple format
 * @param stream the stream on which to do the printing - usually stderr for debugging
 *                     otherwise only downward neighbors will be
 */
void printNodesOnLayer(FILE * stream, int layer_number);

#endif
