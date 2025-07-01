/**
 * @file positions.h
 * @brief data structure and function headers for saving/restoring node order and positions
 *        per layer or for the whole graph
 * The diagram in position_structure.jpg shows details of the data structure.
 * To use it, where my_position_ptr is declared as a Positionptr
 * - my_position_ptr = (Positionptr) calloc(1, sizeof(struct position_struct))
 * - init_position_struct(my_position_ptr)
 * - save and restore position data as needed, either by layer or for the whole graph,
 *   using the save and restore functions defined below
 * - cleanup_position_struct(my_position_ptr)
 * - free(my_position_ptr)
 */

#ifndef POSITIONS_H
#define POSITIONS_H

#include "graph.h"

/**
 * Data structure used to store node position information within layers.
 */
typedef struct node_data_struct {
	Nodeptr node;
	int position;
} NodeData;

/**
 * Keeps track of order information for each node within a layer.
 */
typedef NodeData * LayerDataptr;

/**
 * Holds ordering information for every layer.
 * The main program allocates one of these for saving the best configuration for each objective
 */
typedef struct position_struct {
	LayerDataptr * layer_positions;
} * Positionptr;

/**
 * Copies a layer's current configuration into the layer_position_struct pos_info.
 */
void saveLayerPositions(int layer_number, LayerDataptr layer_info);

/**
 * Copies a graph's current configuration into the position_struct pos_info.
 * Called whenever an objective is improved -- see update_best_x in stats.c
 */
void savePositions(Positionptr pos_info);

/**
 * @brief Sorts the given layer by position
 */
void sortLayerByPosition(int layer_number);

/**
 * @brief Sorts all layers by position
 */
void sortAllLayersByPosition(void);

/**
 * Sets a layer's configuration to match the layer_position_struct pos_info.
 */
void restoreLayerPositions(int layer_number, LayerDataptr layer_info);

/**
 * Sets a graph's configuration to match the position_struct pos_info.
 */
void restorePositions(Positionptr pos_info);

/**
 * Allocates storage for position information.
 */
void init_position_struct(Positionptr pos_info);

/**
 * Deallocates graph position information.
 */
void cleanup_position_struct(Positionptr pos_info);

#endif
