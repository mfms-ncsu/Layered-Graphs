/**
 * @file sifting.c
 * @brief Implementation of functions that place a node in a position on its
 * layer that minimizes the number of crossings or minimizes the maximum
 * number of crossings among edges incident on the node.
 *
 * @author Matt Stallmann
 * @date 2009/01/08
 * $Id: sifting.c 64 2014-03-25 20:36:19Z mfms $
 */

#include"graph.h"
#include"graph_io.h"
#include"defs.h"
#include"heuristics.h"
#include"crossings.h"
#include"crossing_utilities.h"
#include"sifting.h"
#include"swap.h"
#include"sorting.h"
#include"channel.h"
#include"verticality.h"
#include"positions.h"
#include"random.h"

#include<stdio.h>
#include<stdlib.h>
#include<assert.h>
#include<stdbool.h>
#include<limits.h>

/**
 * @brief nodes[leftmost .. rightmost] end up in nodes[leftmost-1 .. rightmost-1];
 *        nodes[rightmost] retains its value
 * @param nodes an array of node pointers; suppose entries are nodes[0 .. k-1]
 * @param leftmost leftmost index of interval to be shifted, must be > 0
 * @param rightmost rightmost index of the interval to be shifted, must be < k
 * nothing happens if rightmost < leftmost 
 */
void leftShift(Nodeptr * nodes, int leftmost, int rightmost) {
  for ( int index = leftmost; index <= rightmost; index++ ) {
    nodes[index - 1] = nodes[index];
  }
}

/**
 * @brief nodes[leftmost .. rightmost] end up in nodes[leftmost+1 .. rightmost+1];
 *        nodes[leftmost] retains its value
 * @param nodes an array of node pointers; suppose entries are nodes[0 .. k-1]
 * @param leftmost leftmost index of interval to be shifted, must be >= 0
 * @param rightmost rightmost index of the interval to be shifted, must be < k-1
 * nothing happens if rightmost < leftmost 
 */
void rightShift(Nodeptr * nodes, int leftmost, int rightmost) {
  for ( int index = rightmost; index >= leftmost; index--) {
    nodes[index + 1] = nodes[index];
  }
}

/**
 * @brief makes sure positions and node indexes coincide on the layer
 * whose number is given
 */
static void updateIndexes(int layer_number) {
  Nodeptr * nodes = layers[layer_number]->nodes;
  int layer_width = layers[layer_number]->number_of_nodes;
  for ( int i = 0; i < layer_width; i++ ) nodes[i]->layer_index = i;
}

/**
 * Puts a node into a different position in an array of nodes.
 * @param node The node to be repositioned
 * @param nodes The array of nodes
 * @param after_position The position of the node that 'node' must come
 * after. If this is -1, then the new position is before all of the other
 * nodes.
 * @attention This does not update indexes!
 * @todo need to clean up sifting, given the different varieties; there are three parts
 * - choose best position for a node, depends on objective
 * - shift nodes to make room
 * - put node into desired position
 * verticality is a special case
 * random sifting is also special in that we need to preserve non-contiguous positions
 */
static void insert_after(Nodeptr node, Nodeptr * nodes,
                             int after_position) {
    // There are three cases to consider: if the node should go immediately
    // after its predecessor or after itself, there is nothing to be done; if
    // it should go immediately after an 'earlier' node, it goes into
    // after_position + 1 and the intervening nodes are shifted right; if it
    // should go after a later node, it goes into after_position and the
    // intervening nodes, including the one it goes after, are shifted left.
    int node_index = node->layer_index;
    if ( after_position < node_index - 1 ) {
      rightShift(nodes, after_position + 1, node_index - 1);
      nodes[after_position + 1] = node;
    }
    else if ( after_position > node_index ) {
      leftShift(nodes, node_index + 1, after_position);
      nodes[after_position] = node;
    }
}

/**
 * @brief puts the node in a position that minimizes the number of crossings.
 *
 * Basic algorithm is as follows:
 *
 * -# let x be the node to be sifted
 * -# for each node y != x, calculate cr(x,y) and cr(y,x), where cr(a,b) is
 * the number of crossings among edges incident to a and b if a and b are in
 * the given order
 * -# use the cr values to compute diff(y) = cr(y,x) - cr(x,y) for each y
 * -# let y_0, ..., y_{L-1} be the nodes other than x on this layer
 * -# let prefix(-1) = cr(x,y_0), prefix(i) = prefix(i-1) + diff(y_i) for i = 0 to L-1
 * -# prefix(i) = # of crossings if x is inserted after the node at index i
 * -# if prefix(i), i >= 0, is minimum over all i, then x is inserted after y_i
 * -# if prefix(-1) is the minimum, then x belongs before y_0
 */
void sift(Nodeptr node)
{
#ifdef DEBUG
  fprintf(stderr, "-> sift, node = %s, layer = %d, position = %d\n",
          node->name, node->layer, node->layer_index);
#endif
  // create an array containing diff(node, y_i) for each y_i on the same
  // layer as 'node', assuming y_i is the node at index i of the layer
  int layer_size =  layers[node->layer]->number_of_nodes;
  Nodeptr * nodes = layers[node->layer]->nodes;
  int * diff = (int *) calloc(layer_size, sizeof(int));
  int i = 0;
  for( i = 0; i < layer_size; i++ ) {
      if ( nodes[i] != node ) {
          diff[i] = crossings_if_first_left_of_second(nodes[i], node)
            - crossings_if_first_left_of_second(node, nodes[i]);
      }
      else {
          diff[i] = 0;
      }
#ifdef DEBUG
      fprintf(stderr, "  sift loop: diff[%d] = %d\n", i, diff[i]);
#endif
   }

  // compute the minimum prefix sum and its position in the diff array
  // bias the decision in favor of maximum distance from the current
  // position; this does consistently better in preliminary experiments,
  // possibly because it's good to cycle through a lot of possible
  // configurations
  int prefix_sum = crossings_if_first_left_of_second(node, nodes[0]);
  int min_prefix_sum = prefix_sum;
  int min_position = -1;
  int max_distance = abs(node->layer_index - nodes[0]->layer_index);
  for( i = 0; i < layer_size; i++ ) {
      prefix_sum += diff[i];
      if( prefix_sum < min_prefix_sum
          || ( prefix_sum == min_prefix_sum
               && abs(i - node->layer_index) > max_distance ) ) {
          min_prefix_sum = prefix_sum;
          min_position = i;
          max_distance = abs(i - node->layer_index);
      }
  }
  free(diff);

  // if min_position is i, then the node belongs between nodes[i] and
  // nodes[i+1];

#ifdef DEBUG
  fprintf(stderr, "   sift, reposition: min_prefix_sum = %d, old = %d, new = %d\n",
          min_prefix_sum, node->layer_index, min_position);
#endif

  insert_after(node, nodes, min_position);
  updateIndexes(node->layer);

  // recompute crossings with respect to this layer
  updateCrossingsForLayer(node->layer);

  // optimize and update verticality
  optimizeLayerVerticality(node->layer, BOTH);

#ifdef DEBUG
  fprintf(stderr, "<- sift, node = %s, layer = %d, position = %d\n",
          node->name, node->layer, node->layer_index);
#endif
}

/**
 * @brief moves the node into a random position on its layer
 * works also for verticality, where not only the index in the array is updated
 * but also the node's position
 */
void randomSift(Nodeptr node) {
  int layer_number = node->layer;
  Nodeptr * layer_nodes = layers[layer_number]->nodes;
  int layer_width = layers[layer_number]->number_of_nodes;
  if ( layer_width < 2 ) return;
  int index_of_node = node->layer_index;
  int random_index = genrand_int31() % layer_width;
  if ( random_index < index_of_node ) rightShift(layer_nodes, random_index, index_of_node -1);
  if ( random_index > index_of_node ) leftShift(layer_nodes, index_of_node + 1, random_index);
  layer_nodes[random_index] = node;
  updateIndexes(layer_number);
  updateCrossingsForLayer(layer_number);
  optimizeLayerVerticality(layer_number, BOTH);
  sortLayerByPosition(layer_number);  // testing
}

/**
 * Algorithm for sifting (a node x) in order to minimize the maximum number of
 * crossings for any edge with one endpoint on its layer:
 *
 * Move to the left and then back to the right (in this case the prefix sum
 * approach doesn't work).  When x is moved to the right of y, you do
 *  - change_crossings( x, y, -1 ): sort with edges of x followed by edges of
 *    y and subtract 1 from the crossing number of an edge when it's involved
 *    in an inversion   
 *  - change_crossings( y, x, +1 ): sort with edges of y followed by edges of
 *    x and add 1 from the crossing number of an edge when it's involved
 *    in an inversion
 *  - among all the edges of x and y, find the one with maximum number of
 *    crossings and use that number as the 'value' of the current position of
 *    x
 * The calculation of inversions needs to be done both for the upward and the
 * downward edges.       
 */

void sift_node_for_edge_crossings( Edgeptr edge, Nodeptr node ) {
  assert( node == edge->up_node || node == edge->down_node );
#ifdef DEBUG
  fprintf(stderr, "-> sift_node_for_edge_crossings: %s -> %s, %s\n",
          edge->down_node->name, edge->up_node->name, node->name);
#endif
  int layer = node->layer;
  int layer_size = layers[ layer ]->number_of_nodes;
  Nodeptr * nodes_on_layer = layers[ layer ]->nodes;

  // find the position where the maximum edge crossing count achieves its
  // minimum; bias the decision in favor of maximum distance from the current
  // position; same strategy applies as with sifting for minimizing
  // overall crossings
  int min_edge_crossing_count = edge->crossings;
  int min_position = node->layer_index;
  int max_distance = 0;
  int current_edge_crossing_count = INT_MAX;

  // begin with a sweep to the left of the current node position
  for ( int i = node->layer_index - 1; i >= 0; i-- ) {
    current_edge_crossing_count
      = edge_crossings_after_swap( nodes_on_layer[i], node );
    if ( current_edge_crossing_count < min_edge_crossing_count
         || ( current_edge_crossing_count == min_edge_crossing_count
              && node->layer_index - i > max_distance )
         ) {
      min_edge_crossing_count = current_edge_crossing_count;
      min_position = i - 1;
      max_distance = node->layer_index - i + 1;
    }
#ifdef DEBUG
    fprintf(stderr, " mce left sweep: pos = %2d, min_pos = %2d, edge xings = %d\n",
            i, min_position, current_edge_crossing_count);
#endif
  }

  // Undo the left sweep (no need to check for min)
  for ( int i = 0; i < node->layer_index; i++ ) {
    edge_crossings_after_swap( node, nodes_on_layer[i] );
#ifdef DEBUG
    printf( " mce undo sweep: pos = %2d, min_pos = %2d, edge xings = %d\n",
            i, min_position, current_edge_crossing_count );
#endif
  }

  // Then sweep all the way to the right
  for ( int i = node->layer_index + 1; i < layer_size; i++ ) {
    current_edge_crossing_count
      = edge_crossings_after_swap( node, nodes_on_layer[i] );
    if ( current_edge_crossing_count < min_edge_crossing_count
         || ( current_edge_crossing_count == min_edge_crossing_count
              && abs(node->layer_index - i) > max_distance )
         ) {
      min_edge_crossing_count = current_edge_crossing_count;
      min_position = i;
      max_distance = abs(node->layer_index - i);
    }
#ifdef DEBUG
    fprintf(stderr, " mce right sweep: pos = %2d, min_pos = %2d, edge xings = %d\n",
            i, min_position, current_edge_crossing_count);
#endif
  }

  insert_after(node, nodes_on_layer, min_position); 
  updateIndexes(layer);

  // recompute crossings with respect to this layer
  updateCrossingsForLayer(layer);

  // and ensure optimum verticality given the new order
  optimizeLayerVerticality(layer, BOTH);
}

extern Positionptr best_verticality_positions;

void sift_node_for_nonverticality(Nodeptr node) {
#ifdef DEBUG
  fprintf(stderr, "-> sift_node_for_nonverticality, node %s\n", node->name);
  writeLayer(stderr, node->layer);
#endif
  int layer = node->layer;

  // find the position where the maximum nonverticality achieves its
  // minimum; bias the decision in favor of maximum distance from the current
  // position; same observation applies as with sifting for minimizing
  // overall crossings
  int min_edge_nonverticality = updateLayerVerticality(layer);
  int initial_position = node->horizontal_position;
  int max_distance = 0;
  int current_edge_nonverticality = INT_MAX;

  // begin with a sweep to the left of the current node position
  for ( int i = initial_position - 1; i >= 0; i-- ) {
#ifdef DEBUG
    fprintf(stderr, " mnve left sweep:"
    " pos = %2d, edge nonverticality = %d, min = %d\n",
            i, current_edge_nonverticality, min_edge_nonverticality);
#endif
    current_edge_nonverticality
      = nonverticality_after_shift(node, i);
    if ( current_edge_nonverticality < min_edge_nonverticality
         || ( current_edge_nonverticality == min_edge_nonverticality
              && abs(node->horizontal_position - i) > max_distance )
         ) {
      min_edge_nonverticality = current_edge_nonverticality;
      saveLayerPositions(layer, best_verticality_positions->layer_positions[layer]);
      max_distance = node->layer_index - i + 1;
    }
  }

  // Undo the left sweep (no need to check for min)
  for ( int i = 1; i <= initial_position; i++ ) {
#ifdef DEBUG
    fprintf(stderr, " mnve undo sweep: pos = %2d, edge nonverticality = %d, min = %d\n",
            i, current_edge_nonverticality, min_edge_nonverticality);
#endif
	  nonverticality_after_shift(node, i);
  }

  // Then sweep all the way to the right
  for ( int i = initial_position + 1; i < max_layer_width; i++ ) {
#ifdef DEBUG
    fprintf(stderr, " mnve rght sweep: pos = %2d, edge nonverticality = %d, min = %d\n",
            i, current_edge_nonverticality, min_edge_nonverticality);
#endif
    current_edge_nonverticality
		= nonverticality_after_shift(node, i);
    if ( current_edge_nonverticality < min_edge_nonverticality
         || ( current_edge_nonverticality == min_edge_nonverticality
              && abs(node->horizontal_position - i) > max_distance )
         ) {
      min_edge_nonverticality = current_edge_nonverticality;
      saveLayerPositions(layer, best_verticality_positions->layer_positions[layer]);
      max_distance = abs(node->layer_index - i);
    }
  }

  // reposition layer using stored data structure
  restoreLayerPositions(layer, best_verticality_positions->layer_positions[layer]);

  // recompute nonverticality with respect to this layer
  updateLayerVerticality(layer);

  // and make sure crossings are correctly computed
  updateIndexes(layer);
  updateCrossingsForLayer(layer);
#ifdef DEBUG
  fprintf(stderr, "<- sift_node_for_nonverticality:\n");
  writeLayer(stderr, layer);
#endif
}

void sift_node_for_total_stretch(Nodeptr node) {
  int layer = node->layer;
  int layer_size = layers[layer]->number_of_nodes;

  if ( layer_size == 1 ) return;

  // resorting to the (possibly inefficient) naive algorithm here, i.e.,
  // recomputing stretch after each move
  double min_stretch = totalLayerStretch(layer);
  int min_position = node->layer_index;
  int original_position = node->layer_index;

  // begin with a sweep to the left of the current node position, keeping
  // track of minimum stretch, or maximum distance as a tie breaker
  for ( int i = original_position - 1; i >= 0; i-- ) {
    swap_nodes(layer, i, i+1);
    double current_stretch = totalLayerStretch(layer);
    if ( current_stretch < min_stretch
         ||
         (current_stretch == min_stretch
          && original_position - i > original_position - min_position) ) {
      min_stretch = current_stretch;
      min_position = i;
    }
#ifdef DEBUG
    fprintf(stderr, " mse left sweep: pos = %2d, min_pos = %2d, stretch = %6.1f\n",
            i, min_position, current_stretch);
#endif
  }

  // sweep right, back to the original position (no need to track stretch)
  for ( int i = 0; i < original_position; i++ ) {
    swap_nodes(layer, i, i+1);
  }

  // sweep to the right of original position, tracking stretch and distance
  for ( int i = original_position + 1; i < layer_size; i++ ) {
    swap_nodes(layer, i-1, i);
    double current_stretch = totalLayerStretch(layer);
    if ( current_stretch < min_stretch
         ||
         (current_stretch == min_stretch
          && i - original_position > abs(original_position - min_position)) ) {
      min_stretch = current_stretch;
      min_position = i;
    }
#ifdef DEBUG
    fprintf(stderr, " mse right sweep: pos = %2d, min_pos = %2d, stretch = %6.1f\n",
            i, min_position, current_stretch);
#endif
  }

  // sweep left to the min position
  for ( int i = layer_size - 1; i > min_position; i-- ) {
    swap_nodes(layer, i-1, i);
  }

  updateCrossingsForLayer(layer);
  optimizeLayerVerticality(layer, BOTH);
} // end, sift node for total stretch
