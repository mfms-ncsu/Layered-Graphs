/**
 * @file verticality.c
 * @author Matthias Stallmann (mfms@ncsu.edu)
 * @brief implementation of functions related to maintaining verticality information
 * 
 * @copyright Copyright (c) 2022
 */

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include "graph.h"
#include "verticality.h"
#include "heuristics.h"
#include "defs.h"
#include "random.h"

static void updateEdgeVerticality( Edgeptr );

/**
 * @brief initializes the nonverticality of all edges in the graph
 */
void initializeVerticality(void) {
	// Update all edges' verticality
	updateAllVerticality();
}

/**
  * @return the total nonverticality of all edges
  * @assume !!! updates have been done;
  *             this is the case only if a heuristic addressing verticality is used
 */
long getTotalNonverticality(void) {
	long nonverticality = 0;
	// retrieve stored (non-)verticality of each edge
	for ( unsigned int i = 0; i < number_of_edges; i++ ) {
		// Get a particular edge
		Edgeptr edge = master_edge_list[i];
		nonverticality += edge->nonverticality;
	}
	return nonverticality;
}

int getBottleneckNonverticality(void) {
	int bottleneck_nonverticality = 0;
	// retrieve stored (non-)verticality of each edge
	for ( unsigned int i = 0; i < number_of_edges; i++ ) {
		// Get a particular edge
		Edgeptr edge = master_edge_list[i];
		if ( edge->nonverticality > bottleneck_nonverticality )
			bottleneck_nonverticality = edge->nonverticality;
	}
	return bottleneck_nonverticality;
}

/**
 * @brief updates nonverticality of all edges in the graph
 * @return the total nonverticality of all edges
 */
long updateAllVerticality(void) {
#ifdef DEBUG
	fprintf(stderr, "-> updateAllVerticality\n");
#endif
	long nonverticality = 0;
	// Calculate (non-)verticality on each edge
	for ( unsigned int i = 0; i < number_of_edges; i++ ) {
		// Get a particular edge
		Edgeptr edge = master_edge_list[ i ];

		// Verify edge
		if ( edge ) {
			// Update edge nonverticality
			updateEdgeVerticality( edge );
			nonverticality += edge->nonverticality;
		}
	}
#ifdef DEBUG
	fprintf(stderr, "<- updateAllVerticality, nonverticality = %ld\n", nonverticality);
#endif
	return nonverticality;
}

/**
 * @brief updates nonverticality of edges incident on nodes of the layer
 * @return the total nonverticality of edges incident on the layer
 */
long updateLayerVerticality(int layer_number) {
	long nonverticality = 0;
#ifdef DEBUG
	printf("-> updateLayerVerticality, layer_number = %d\n", layer_number);
#endif
	Layerptr layer = layers[layer_number];
	// Calculate (non-)verticality on edges incident to each node
	for ( unsigned int i = 0; i < layer->number_of_nodes; i++ ) {
		// Get a particular node
		Nodeptr node = layer->nodes[ i ];

		// Verify node
		if ( node ) {
			// Update node nonverticality
			nonverticality += updateNodeVerticality(node, BOTH);
		}
	}
#ifdef DEBUG
	printf("<- updateLayerVerticality, nonverticality = %ld\n", nonverticality);
#endif
	return nonverticality;
}

/**
 * @brief updates nonverticality of a particular edge in the graph
 * @todo to be consistent, this one should also return a value
 */
static void updateEdgeVerticality(Edgeptr edge) {
#ifdef DEBUG
	fprintf(stderr, "-> updateEdgeVerticality, edge: %d %d\n",
					edge->up_node->id, edge->down_node->id);
#endif
	// For consistency, these algorithms will refer to the
	// upper node as the "x" node and the lower as the "y"
	// Calculate x, y node positions
	int position_x = edge->up_node->horizontal_position;
	int position_y = edge->down_node->horizontal_position;
#ifdef DEBUG
	fprintf(stderr, "  positions: %d %d\n", position_x, position_y);
#endif

	// Calculate difference between x, y node positions
	int diff = position_x - position_y;

	// Update edge nonverticality
	edge->nonverticality = diff * diff;

#ifdef DEBUG
	fprintf(stderr, "<- updateEdgeVerticality, nonverticality = %d\n", edge->nonverticality);
#endif
}

long updateNodeVerticality(Nodeptr node, Orientation direction) {
#ifdef DEBUG
	printf("-> updateNodeVerticality for node %d\n", node->id);
#endif
	long nonverticality = 0;
	// Calculate (non-)verticality on each upper edge; omit if direction is down
	if ( direction != DOWNWARD ) {
		for ( unsigned int i = 0; i < node->up_degree; i++ ) {
			Edgeptr edge = node->up_edges[i];
			updateEdgeVerticality( edge );
			nonverticality += edge->nonverticality;
		}
	}

	// Calculate (non-)verticality on each lower edge; omit if direction is up
	if ( direction != UPWARD ) {
		for ( unsigned int i = 0; i < node->down_degree; i++ ) {
			Edgeptr edge = node->down_edges[i];
			updateEdgeVerticality( edge );
			nonverticality += edge->nonverticality;
		}
	}
#ifdef DEBUG
	printf("<- updateNodeVerticality for node %d, nonverticality = %ld\n",
	       node->id, nonverticality);
#endif
	return nonverticality;
}

/**
 * @brief finds the layer with the maximum non-verticality
 * @return The number of an unfixed layer whose incident edges have the largest total non-verticality, or -1 if layers are fixed.
 */
int maxNonverticalityLayer(void) {
#ifdef DEBUG
	fprintf(stderr, "-> minVerticalityLayer\n");
#endif
	int max_nonverticality_layer = -1;
	int max_nonverticality = -1;
	for ( int i = 0; i < number_of_layers; i++ ) {
		int non_verticality = updateLayerVerticality(i);
		if ( non_verticality > max_nonverticality
				&& ! isFixedLayer(i) ) {
			max_nonverticality = non_verticality;
			max_nonverticality_layer = i;
		}
	}
#ifdef DEBUG
	fprintf(stderr, "<- minVerticalityLayer, layer = %d, nv = %d\n",
	max_nonverticality_layer, max_nonverticality);
#endif
	return max_nonverticality_layer;
}


/**
 * @brief finds the edge with the maximum non-verticality
 * @return The number of an unfixed edge with the largest non-verticality, or NULL if all edges are fixed.
 */
Edgeptr maxNonverticalityEdge(void) {
#ifdef DEBUG
	fprintf(stderr, "-> maxNonverticalityEdge\n");
#endif
	Edgeptr max_nonverticality_edge = NULL;
	int max_nonverticality = -1;
	for ( int i = 0; i < number_of_edges; i++ ) {
		Edgeptr edge = master_edge_list[i];
		if ( edge->nonverticality > max_nonverticality
				&& ! isFixedEdge( edge ) ) {
			max_nonverticality = edge->nonverticality;
			max_nonverticality_edge = edge;
		}
	}
#ifdef DEBUG
	if ( max_nonverticality_edge != NULL ) {
		fprintf(stderr, "<- maxNonverticalityEdge, edge %s -> %s, nv = %d\n",
				max_nonverticality_edge->down_node->name,
				max_nonverticality_edge->up_node->name, max_nonverticality);
	}
	else {
		fprintf(stderr, "<- maxNonverticalityEdge, edge = NULL\n");
	}
#endif
	return max_nonverticality_edge;
}

/**
 * @brief sets positions of the nodes so that they are centered wrt maximum width
 * 
 * @param nodes an array of nodes on a layer
 * @param number_of_nodes number of nodes in the array, layer width
 * @param max_width maximum width of any layer
 */
void centerNodes(int layer_number) {
	Nodeptr * nodes = layers[layer_number]->nodes;
	int number_of_nodes = layers[layer_number]->number_of_nodes;
	int current_position = (max_layer_width - number_of_nodes) / 2;
	for ( int i = 0; i < number_of_nodes; i++ ) {
		nodes[i]->horizontal_position = current_position++;
	}
}

/**
 * @brief optimizes the verticality of the layer using a dynamic programming algorithm
 *        and recalculates verticalities for nodes on the layer
 * 
 * @param layer_index index of the relevant layer
 * @param direction whether the optimization is to be based on verticalities
 * 					of edges going upward, downward or both
 */
void optimizeLayerVerticality(int layer_index, Orientation direction) {
	dynamicProgramming(layers[layer_index], direction);
	updateLayerVerticality(layer_index);
}

/**
 * @brief centers nodes on every layer with respect to maximum layer width
 */
void centerAllLayers(void) {
	for ( int layer_number = 0; layer_number < number_of_layers; layer_number++ ) {
		centerNodes(layer_number);
	}
}

/**
 * @brief uses centering and dynamic programming to find the best verticality positions
 * for all nodes without changing the order on any layer
 * optimization is done via two barycenter sweeps
 * @remark A max_width_layer effectively "blocks" the optimization if you do
 *         it downward from the layer above or upward from the layer below
 *         doing the second sweep based on both directions mitigates this effect
 * @attention this is called only after preprocessing, whether or not a preprocessor is used
 */
void adjustVerticalities(void) {
	centerAllLayers();
	for ( int layer = 1; layer < number_of_layers; layer++ ) {
    	optimizeLayerVerticality(layer, DOWNWARD);
	}
	for ( int layer = number_of_layers - 2; layer >= 0; layer-- ) {
		optimizeLayerVerticality(layer, UPWARD);
	}

	for ( int layer = 1; layer < number_of_layers; layer++ ) {
    	optimizeLayerVerticality(layer, BOTH);
	}
	for ( int layer = number_of_layers - 2; layer >= 0; layer-- ) {
		optimizeLayerVerticality(layer, BOTH);
	}
}