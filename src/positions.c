/**
 * @file positions.c
 * @brief implementation of functions for saving/restoring node order and positions
 *        by layer or for the whole graph.
 * @author Matt Stallmann
 */

#include"positions.h"
#include"sorting.h"
#include"graph.h"
#include"graph_io.h"

#ifdef DEBUG
#include"crossings.h"
#endif

#include<stdio.h>
#include<stdlib.h>
#include<assert.h>
#include<stdbool.h>

void init_position_struct(Positionptr pos_info) {
	// allocate layer_positions
    pos_info->layer_positions = (LayerDataptr *) calloc(number_of_layers, sizeof(LayerDataptr));

	// allocate each layer_position_struct
	for ( int i = 0; i < number_of_layers; i++ ) {
    	size_t nodes_count = layers[i]->number_of_nodes;
    	size_t size = sizeof(NodeData);
    	pos_info->layer_positions[i] = (NodeData *) calloc( nodes_count, size );
	}

	savePositions(pos_info);
}

void saveLayerPositions(int layer_number, LayerDataptr layer_info) {
#ifdef DEBUG
	fprintf(stderr, "-> saveLayerPositions, checking if layer is sorted\n");
	sortLayerByPosition(layer_number);	//for debugging
#endif
	// get layer being stored
	Layerptr layer = layers[layer_number];
	// assign each node in the layer to its corresponding
	// layer_info index
	for ( int i = 0; i < layer->number_of_nodes; i++ ) {
		// get node and storage being assigned
		Nodeptr node = layer->nodes[i];

		layer_info[i].node = node;
		layer_info[i].position = node->horizontal_position;
		if ( i > 0 && layer_info[i].position == layer_info[i-1].position ) {
			fprintf(stderr, "*** Error: two nodes have the same position\n");
			writeNode(stderr, node);
			fprintf(stderr, " <=> ");
			writeNode(stderr, layer_info[i-1].node);
			abort();
		}
	}
#ifdef DEBUG
	fprintf(stderr, "<- saveLayerPositions, layer is sorted\n");
#endif
}

void savePositions(Positionptr pos_info) {
	// save each layer position
	for ( int i = 0; i < number_of_layers; i++ )
    	saveLayerPositions(i, pos_info->layer_positions[i]);
}

void restoreLayerPositions(int layer_number, LayerDataptr layer_info) {
#ifdef DEBUG
	fprintf(stderr, "-> restoreLayerPositions\n");
#endif
	// get layer being rearranged
	Layerptr layer = layers[layer_number];

	// update Layerptr nodes
	for ( int i = 0; i < layer->number_of_nodes; i++ ) {
		layer->nodes[i] = layer_info[i].node;
		layer->nodes[i]->horizontal_position = layer_info[i].position;
		if ( i > 0 && layer_info[i].position == layer_info[i-1].position ) {
			fprintf(stderr, "*** Error: two nodes have the same position\n");
			writeNode(stderr, layer_info[i].node);
			fprintf(stderr, " <=> ");
			writeNode(stderr, layer_info[i-1].node);
			abort();
		}
	}
#ifdef DEBUG
	fprintf(stderr, "<- restoreLayerPositions, checking if layer is sorted\n");
	sortLayerByPosition(layer_number);	//for debugging
#endif
}

void restorePositions(Positionptr pos_info) {
	// restore each layer in pos_info
	for ( int i = 0; i < number_of_layers; i++ ) {
		restoreLayerPositions(i, pos_info->layer_positions[i]);
	}
}

int compare_positions(const void * ptr_1, const void * ptr_2) {
  const Nodeptr * entry_ptr_1 = (Nodeptr *) ptr_1;
  const Nodeptr * entry_ptr_2 = (Nodeptr *) ptr_2;
  const Nodeptr node_1 = * entry_ptr_1;
  const Nodeptr node_2 = * entry_ptr_2;
  int position_1 = node_1->horizontal_position;
  int position_2 = node_2->horizontal_position;
  // can't have two nodes in the same position on a layer
  if ( position_1 == position_2 ) {
	fprintf(stderr, "*** Error: two nodes have same position on layer ***\n");
	writeNode(stderr, node_1);
	writeNode(stderr, node_2);
	abort();
  }
  return position_1 - position_2;
}

/**
 * @brief Sorts the give layer structure by position
 */
void sortLayerByPosition(int layer_number) {
#ifdef DEBUG
	fprintf(stderr, "-> sortLayerByPosition\n");
	writeLayer(stderr, layer_number);
#endif
	Layerptr layer = layers[layer_number];
	int num_nodes = layer->number_of_nodes;
	Nodeptr * nodes = layer->nodes;	
	insertionSort(nodes, num_nodes, sizeof(Nodeptr), compare_positions);
#ifdef DEBUG
	fprintf(stderr, "<- sortLayerByPosition\n");
	writeLayer(stderr, layer_number);
#endif
}

/**
 * @brief Sorts the give layer structure by position
 */
void sortAllLayersByPosition(void) {
	for ( int layer_number  = 0; layer_number < number_of_layers; layer_number++) {
		sortLayerByPosition(layer_number);
	}
}

void cleanup_position_struct(Positionptr pos_info) {
    for ( int i = 0; i < number_of_layers; i++ )
        free(pos_info->layer_positions[i]);
    free(pos_info->layer_positions);
}
