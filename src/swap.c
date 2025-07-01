/**
 * @file swap.c
 * @brief Implementation of functions that compute the change in crossing
 * number (or max edge crossings if desired) when two neighboring nodes are swapped.
 * @author Matt Stallmann
 * @date 2011/05/21
 * $Id: swap.c 2 2011-06-07 19:50:41Z mfms $
 */

#include"graph.h"
#include"graph_io.h"
#include"defs.h"
#include"crossings.h"
#include"crossing_utilities.h"
#include"swap.h"
#include"sorting.h"
#include"verticality.h"
#include"positions.h"
#include"heuristics.h"

#include<stdio.h>
#include<stdlib.h>
#include<assert.h>
#include<stdbool.h>
#include<limits.h>

/**
 * @brief copy all the information from source_node to destination_node
 */
static void copy_node(Nodeptr destination_node, Nodeptr source_node) {

}

/**
 * swap the nodes in positions i and j on the given layer, making sure that the
 * layer indexes and horizontal positions are also swapped
 */
void swap_nodes(int layer, int i, int j) {
  Nodeptr * nodes_on_layer = layers[layer]->nodes;
#ifdef DEBUG
  fprintf(stderr, "-> swap_nodes, layer = %d, i = %d, j = %d", layer, i, j);
  fprintf(stderr, ", nodes[%d] = %d, nodes[%d] = %d\n",
          i, nodes_on_layer[i]->id, j, nodes_on_layer[j]->id);
  fprintf(stderr, "    index[%d] = %d, index[%d] = %d, position[%d] = %d, position[%d] = %d\n",
          i, nodes_on_layer[i]->layer_index, j, nodes_on_layer[j]->layer_index,
          i, nodes_on_layer[i]->horizontal_position, j, nodes_on_layer[j]->horizontal_position);
#endif
  assert(i >= 0 && j >= 0);
  assert(i < layers[layer]->number_of_nodes && i < layers[layer]->number_of_nodes);
//  Nodeptr tmp_node = (Nodeptr) malloc(sizeof(struct node_struct));
  // swap position values first to avoid confusion
  int tmp_position_i = nodes_on_layer[i]->horizontal_position;
  int tmp_position_j = nodes_on_layer[j]->horizontal_position;
  nodes_on_layer[i]->horizontal_position = tmp_position_j;
  nodes_on_layer[j]->horizontal_position = tmp_position_i;
  // now swap nodes in the array of nodes on layer
  Nodeptr tmp = nodes_on_layer[i];
  nodes_on_layer[i] = nodes_on_layer[j];
  nodes_on_layer[j] = tmp;
  // finally, fix the layer indexes
  nodes_on_layer[i]->layer_index = i;
  nodes_on_layer[j]->layer_index = j;
#ifdef DEBUG
  fprintf(stderr, "<- swap_nodes");
  fprintf(stderr, ", nodes[%d] = %d, nodes[%d] = %d\n",
          i, nodes_on_layer[i]->id, j, nodes_on_layer[j]->id);
  fprintf(stderr, "    index[%d] = %d, index[%d] = %d, position[%d] = %d, position[%d] = %d\n",
          i, nodes_on_layer[i]->layer_index, j, nodes_on_layer[j]->layer_index,
          i, nodes_on_layer[i]->horizontal_position, j, nodes_on_layer[j]->horizontal_position);
#endif
} 

/**
 * Fills edge_array with upward edges of the two nodes, each set of edges
 * sorted by their endpoint positions on the layer above and those of the
 * first node coming first. Used to compute number of swaps by counting
 * inversions [reference to paper by Jünger and Mutzel needed]
 */
void create_sorted_up_edge_array( Edgeptr * edge_array,
                                  Nodeptr first_node,
                                  Nodeptr second_node );

/**
 * Fills edge_array with downward edges of the two nodes, each set of edges
 * sorted by their endpoint positions on the layer above and those of the
 * first node coming first. Used to compute number of swaps by counting
 * inversions [reference to paper by Jünger and Mutzel needed]
 */
void create_sorted_down_edge_array( Edgeptr * edge_array,
                                    Nodeptr first_node,
                                    Nodeptr second_node );

int edge_crossings_for_node( Nodeptr node )
{
  int edge_crossings = 0;
  // update max_edge_crossings based on upward edges
  for ( int i = 0; i < node->up_degree; i++ )
    if ( node->up_edges[i]->crossings > edge_crossings )
      edge_crossings = node->up_edges[i]->crossings;
  // update max_edge_crossings based on downward edges
  for ( int i = 0; i < node->down_degree; i++ )
    if ( node->down_edges[i]->crossings > edge_crossings )
      edge_crossings = node->down_edges[i]->crossings;
  return edge_crossings;
}

int edge_nonverticality_for_node( Nodeptr node )
{
  updateNodeVerticality(node, BOTH);
  int edge_nonverticality = 0;
  // update max_edge_crossings based on upward edges
  for ( int i = 0; i < node->up_degree; i++ )
    if ( node->up_edges[i]->nonverticality > edge_nonverticality )
      edge_nonverticality = node->up_edges[i]->nonverticality;
  // update max_edge_crossings based on downward edges
  for ( int i = 0; i < node->down_degree; i++ )
    if ( node->down_edges[i]->nonverticality > edge_nonverticality )
      edge_nonverticality = node->down_edges[i]->nonverticality;
  return edge_nonverticality;
}

int crossings_if_first_left_of_second( Nodeptr node_a, Nodeptr node_b )
{
  assert( node_a->layer == node_b->layer );
  int layer = node_a->layer;

  int total_crossings = 0;

  // count crossings among upward edges (if any)
  if ( layer < number_of_layers - 1 )
    {
      Edgeptr * edge_array
        = (Edgeptr *) calloc( node_a->up_degree + node_b->up_degree,
                              sizeof(Edgeptr) );
      create_sorted_up_edge_array( edge_array, node_a, node_b );
      total_crossings += count_inversions_up( edge_array,
                                              node_a->up_degree
                                              + node_b->up_degree, 1 );
      free( edge_array );
    }

  // count crossings among downward edges (if any)
  if ( layer > 0 )
    {
      Edgeptr * edge_array
        = (Edgeptr *) calloc( node_a->down_degree + node_b->down_degree,
                              sizeof(Edgeptr) );

      create_sorted_down_edge_array( edge_array, node_a, node_b );
      total_crossings += count_inversions_down( edge_array,
                                                node_a->down_degree
                                                + node_b->down_degree, 1 );
      free( edge_array );
    }
  return total_crossings;
}

void change_crossings( Nodeptr left_node, Nodeptr right_node, int diff )
{
  int layer = left_node->layer;

  // update crossings on upward edges (if any)
  if ( layer < number_of_layers - 1 )
    {
      Edgeptr * edge_array
        = (Edgeptr *) calloc( left_node->up_degree + right_node->up_degree,
                              sizeof(Edgeptr) );
      create_sorted_up_edge_array( edge_array, left_node, right_node );
      count_inversions_up( edge_array,
                           left_node->up_degree + right_node->up_degree,
                           diff );
      free( edge_array );
    }

  // update crossings on downward edges (if any)
  if ( layer > 0 )
    {
      Edgeptr * edge_array
        = (Edgeptr *) calloc( left_node->down_degree + right_node->down_degree,
                              sizeof(Edgeptr) );
      create_sorted_down_edge_array( edge_array, left_node, right_node );
      count_inversions_down( edge_array,
                             left_node->down_degree + right_node->down_degree,
                             diff );
      free( edge_array );
    }
}

int edge_crossings_after_swap( Nodeptr left_node, Nodeptr right_node )
{
  change_crossings( left_node, right_node, -1 );
  change_crossings( right_node, left_node, +1 );
  int left_node_edge_crossings = edge_crossings_for_node( left_node );
  int right_node_edge_crossings = edge_crossings_for_node( right_node );
  if( left_node_edge_crossings > right_node_edge_crossings )
    return left_node_edge_crossings;
  else
    return right_node_edge_crossings;
}

/**
 * @param node the node to shifted 
 * @param target_position the position to which it will be shifted
 * @return the total nonverticality of the layer after the node is shifted
 * @pre nodes on the layer must be sorted by position
 * target position must be immediately to the left or right of the node's current position
 */
int nonverticality_after_shift(Nodeptr node, int target_position) {
  int layer_index = node->layer;
	Layerptr layer = layers[layer_index];
  int node_index = node->layer_index;

#ifdef DEBUG
  fprintf(stderr, "-> nonverticality_after_shift, node_name = %s, node_index = %d,"
                  " target_pos = %d\n",
          node->name, node_index, target_position);
#endif

	// get next left and right nodes
	Nodeptr left_node = NULL, right_node = NULL;

	if ( node_index - 1 >= 0 ) left_node = layer->nodes[node_index - 1];
	if ( node_index + 1 < layer->number_of_nodes ) right_node = layer->nodes[node_index + 1];

	// swap node with left or right node if left or right node in target position
	if ( left_node && left_node->horizontal_position == target_position ) {
		// left node needs to move to accomodate node
		left_node->horizontal_position = node->horizontal_position;

		// swap node in layer array and make sure indexes are updated
		layer->nodes[node_index - 1] = node;
		layer->nodes[node_index] = left_node;
    node->layer_index = node_index - 1;
    left_node->layer_index = node_index;
#ifdef DEBUG
    fprintf(stderr, "    <-> swap with left node "); writeNode(stderr, left_node);
#endif
	}
  else if ( right_node && right_node->horizontal_position == target_position ) {
		// right node needs to move
		right_node->horizontal_position = node->horizontal_position;

		// swap node in layer array and make sure indexes are updated
		layer->nodes[node_index + 1] = node;
		layer->nodes[node_index] = right_node;
    node->layer_index = node_index + 1;
    right_node->layer_index = node_index;
#ifdef DEBUG
    fprintf(stderr, "    <-> swap with right node "); writeNode(stderr, right_node);
#endif
	}
  node->horizontal_position = target_position;
#ifdef DEBUG
  fprintf(stderr, "<- nonverticality_after_shift, position = %d\n",
          node->horizontal_position);
#endif

	return updateLayerVerticality(layer_index);
}

void create_sorted_up_edge_array( Edgeptr * edge_array,
                                  Nodeptr first_node,
                                  Nodeptr second_node )
{
  sortByUpNodePosition( first_node->up_edges, first_node->up_degree );
  sortByUpNodePosition( second_node->up_edges, second_node->up_degree );
  add_edges_to_array( edge_array,
                      first_node->up_edges, first_node->up_degree, 0 );
  add_edges_to_array( edge_array,
                      second_node->up_edges, second_node->up_degree,
                      first_node->up_degree );
}

void create_sorted_down_edge_array( Edgeptr * edge_array,
                                    Nodeptr first_node,
                                    Nodeptr second_node )
{
  sortByDownNodePosition( first_node->down_edges, first_node->down_degree );
  sortByDownNodePosition( second_node->down_edges, second_node->down_degree );
  add_edges_to_array( edge_array,
                      first_node->down_edges, first_node->down_degree, 0 );
  add_edges_to_array( edge_array,
                      second_node->down_edges, second_node->down_degree,
                      first_node->down_degree );
}
