/**
 * @file dynamic_programming.c
 * @author Matt Stallmann
 * @brief implementation of optimal dynamic programming algorithm to minimize nonverticality
 *        of a sorted layer
 * @date 2022-04-26
 */

#include<stdio.h>
#include<stdlib.h>
#include<limits.h>

#include"graph.h"
#include"dynamic_programming.h"
#include"verticality.h"

/**
 * global variables to avoid passing parameters
 */
Layerptr this_layer = NULL;
int layer_width = 0;

static unsigned long * v_cost = NULL;
static unsigned long * cost = NULL;
static bool * at_position = NULL;

/**
 * AT_INDEX(A, node_index, position) is the (address of) A[node_index][position]
 * where A is a dynamically allocated one-dimensional array that accomodates
 * the largest possible layer width
 * 
 * The three relevant arrays are - where v is the node at node_index and p its position
 * - v_cost = nonverticality if v is put into position p,
 *       based on positions of nodes on neighboring layers
 * - cost = minimum verticality for nodes at indices 0 , ... , node_index
 *       using positions 0 , ... p
 * - at_position = true if the cost at node_index, position is achieved
 */
#define AT_INDEX(A, node_index, position) (A[(node_index) * max_layer_width + (position)])

/**
 * @brief fills the v_cost array using nonverticality information about nodes
 *        in each position
 * does so for each node by
 *   - saving its horizontal position
 *   - calculating its nonverticality at all relevant positions
 *   - restoring its horizontal position
 * The relevant positions must be
 *   - >= the index of the node; otherwise no room for node and its predecessors
 *   - < max_layer_width - # of nodes after this one; otherwise no room for the later nodes
 *       # of nodes after = layer_width - node_index
 * @param layer the array of nodes on the relevant layer
 * @param direction indicates whether cost is to be based on verticality
 *                  of edges going upward, downward, or both
 */
static void init_v_cost(Layerptr layer, Orientation direction) {
#ifdef DEBUG
    printf("-> init_v_cost, layer_width = %d, max = %d\n",
           layer_width, max_layer_width);
#endif
    for ( int node_index = 0;
              node_index < layer_width;
              node_index++ ) {
#ifdef DEBUG
        printf(" - costs for node %d\n", node_index);
#endif
        Nodeptr node = layer->nodes[node_index];
        int save_position = node->horizontal_position;
        for ( int position = node_index;
                  position <= max_layer_width - (layer_width - node_index);
                  position++ ) {
            node->horizontal_position = position;
            AT_INDEX(v_cost, node_index, position) = updateNodeVerticality(node, direction);
#ifdef DEBUG
            printf(" v_cost, node %d, position %d =\t%10lu\n",
                   node_index, position, AT_INDEX(v_cost, node_index, position));
#endif
        }
        node->horizontal_position = save_position;
    }
}

/**
 * @brief recursively computes the position of each node using the at_position table
 * This is done after the dynamic programming algorithm is executed;
 * standard procedure for recovering a solution
 */
static void compute_positions(int node_index, int position) {
    // run out of positions
    if ( node_index == -1 || position == -1 ) return;
#ifdef DEBUG
    printf("-> compute_positions, node = %3d, position = %3d\n", node_index, position);
#endif
    if ( AT_INDEX(at_position, node_index, position) ) {
        this_layer->nodes[node_index]->horizontal_position = position;
#ifdef DEBUG
        printf("  *\t\tnode %3d, position %3d, v_cost %7lu\n",
               node_index, position, AT_INDEX(v_cost, node_index, position));
#endif
        compute_positions(node_index - 1, position - 1); 
    }
    else {
        compute_positions(node_index, position - 1);
    }
}

/**
 * @brief dynamic programming algorithm to minimize nonverticality
 *        of the layer while maintaining the order of the nodes
 * based on solution to CSC 505 homework3 problem in 2017
 */
void dynamicProgramming(Layerptr layer, Orientation direction) {
    this_layer = layer;
    layer_width = layer->number_of_nodes;
    init_v_cost(layer, direction);

    // node 0 is a special case because there is no node -1
    AT_INDEX(cost, 0, 0) = AT_INDEX(v_cost, 0, 0);
    AT_INDEX(at_position, 0, 0) = true;
#ifdef DEBUG
    printf(" cost for node   0, position   0 = %10lu *\n", AT_INDEX(cost, 0, 0));
#endif
    for ( int position = 1; position <= max_layer_width - layer_width; position++ ) {
        AT_INDEX(cost, 0, position) = AT_INDEX(v_cost, 0, position);
        AT_INDEX(at_position, 0, position) = true;
        if ( AT_INDEX(cost, 0, position - 1) < AT_INDEX(cost, 0, position) ) {
            AT_INDEX(cost, 0, position) = AT_INDEX(cost, 0, position - 1);
            AT_INDEX(at_position, 0, position) = false;
        }
#ifdef DEBUG
    {
        char * at_position_indicator
              = AT_INDEX(at_position, 0, position) ? "*" : "";
        printf(" cost for node %3d, position %3d = %10lu %s\n",
               0, position, AT_INDEX(cost, 0, position), at_position_indicator);
    }
#endif
    }

    // main DP algorithm
    for ( int node_index = 1; node_index < layer_width; node_index++ ) {
        // putting node at position corresponding to its index implies doing the same for previous nodes
        AT_INDEX(cost, node_index, node_index)
            = AT_INDEX(cost, node_index - 1, node_index - 1)
            + AT_INDEX(v_cost, node_index, node_index);
        AT_INDEX(at_position, node_index, node_index) = true;
#ifdef DEBUG
        printf(" cost for node %3d, position %3d = %10lu *\n",
               node_index, node_index, AT_INDEX(cost, node_index, node_index));
#endif
        // now consider the other cases
        for ( int position = node_index + 1;
                  position <= max_layer_width - (layer_width - node_index); position++ ) {
            // default is not putting node at position, so must be to the left
            AT_INDEX(cost, node_index, position) = AT_INDEX(cost, node_index, position - 1);
            AT_INDEX(at_position, node_index, position) = false;
            unsigned long cost_if_node_at_position
                = AT_INDEX(cost, node_index - 1, position - 1)
                + AT_INDEX(v_cost, node_index, position);
            if ( cost_if_node_at_position < AT_INDEX(cost, node_index, position) ) {
                AT_INDEX(cost, node_index, position) = cost_if_node_at_position;
                AT_INDEX(at_position, node_index, position) = true;
            }
#ifdef DEBUG
        {
            char * at_position_indicator
                 = AT_INDEX(at_position, node_index, position) ? "*" : "";
            printf(" cost for node %3d, position %3d = %10lu %s\n",
                 node_index, position, AT_INDEX(cost, node_index, position),
                 at_position_indicator);
        }
#endif
        }
    }

    // compute final positions
    compute_positions(layer_width -  1, max_layer_width - 1);
}

/**
 * @brief allocates memory for the dynamic programming algorithm
 *        three matrices large enough to accomodate the largest layer
 */
void allocateDPMatrices(void) {
    v_cost = (unsigned long *) calloc(max_layer_width * max_layer_width, sizeof(unsigned long));
    cost = (unsigned long *) calloc(max_layer_width * max_layer_width, sizeof(unsigned long));
    at_position = (bool *) calloc(max_layer_width * max_layer_width, sizeof(bool));
}

/**
 * @brief deallocates memory used by the dynamic programming algorithm
 */
void deallocateDPMatrices(void) {
    free(v_cost);
    free(cost);
    free(at_position);
}
