/**
 * @file swap.h
 * @brief Headers of functions that compute the change in crossing
 * number or max edge crossings when two neighboring nodes are swapped; the
 * only function used by heuristics that minimize total crossings is
 * node_crossings()
 *
 * @author Matt Stallmann
 * @date 2011/05/21
 * $Id: swap.h 2 2011-06-07 19:50:41Z mfms $
 */

#ifndef SWAP_H
#define SWAP_H

#include"defs.h"
#include"graph.h"

/**
 * swap the nodes in positions i and j on the given layer
 */
void swap_nodes(int layer, int i, int j);

/**
 * @return the number of crossings among the edges of node_a and node_b if
 * node_a is to the left of node_b
 */
int crossings_if_first_left_of_second( Nodeptr node_a, Nodeptr node_b );

/**
 * Change counts based on crossings when left_node appears to the left and
 * right node to the right.
 * @param diff +1 to increase crossing counts, -1 to decrease
 * - used by mce heuristic only
 */
void change_crossings( Nodeptr left_node, Nodeptr right_node, int diff );

/**
 * @return the maximum number of edge crossings involving this node
 * - used by mce heuristic only
 */
int edge_crossings_for_node( Nodeptr node );

/**
 * @return the maximum nonverticality involving this node
 * @remark not currently used; might be for a bottleneck heuristic
 */
int edge_nonverticality_for_node( Nodeptr node );

/**
 * @return the maximum number of crossings for an edge incident on left_node
 * or right_node after the two are swapped. *** Side effect: crossings on all
 * relevant nodes and edges are updated in accordance with the swap. ***
 * - used by mce heuristic only
 */
int edge_crossings_after_swap( Nodeptr left_node, Nodeptr right_node );

/**
 * @param node the node to shifted 
 * @param target_position the position to which it will be shifted
 * @return the total nonverticality of the layer after the node is shifted
 * @pre nodes on the layer must be sorted by position
 * target position must be immediately to the left or right of the nodes current position
 */
int nonverticality_after_shift(Nodeptr node, int target_position);

#endif
