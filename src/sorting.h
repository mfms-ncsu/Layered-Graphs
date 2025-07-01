/**
 * @file sorting.h
 * @brief Interface for functions that perform various sorts. All sorts are
 * stable.
 * @author Matthias Stallmann
 * @date 2009/01/03
 * $Id: sorting.h 28 2011-07-18 19:52:04Z mfms $
 */

#ifndef SORTING_H
#define SORTING_H

#include<stddef.h>

#include"graph.h"

/**
 * Performs an insertion sort using the same argument types as qsort
 * @return true if the original order has changed
 */
bool insertionSort(void *base, size_t nmemb, size_t size,
                           int (*compar)(const void *, const void *));

/**
 * Sorts the nodes of the given layer by increasing weight and updates the
 * position fields of the nodes accordingly. Uses insertion sort.
 */
void layerSort( int layer );

/**
 * Sorts the nodes of the given layer by increasing weight and updates the
 * position fields of the nodes accordingly. Uses merge sort.
 */
void layerMergesort( int layer );

/**
 * Sorts the nodes of the given layer by increasing weight and updates the
 * position fields of the nodes accordingly. Uses insertion sort with equal
 * elements in reverse order.
 */
void layerUnstableSort( int layer );

/**
 * Sorts the nodes of the given layer by increasing weight and updates the
 * position fields of the nodes accordingly. Uses Quicksort.
 */
void layerQuicksort( int layer );

/**
 * Sorts the array of edges by the positions of the nodes on the lower layer
 */
void sortByDownNodePosition( Edgeptr * edge_array, int num_edges );

/**
 * Sorts the array of edges by the positions of the nodes on the upper layer
 */
void sortByUpNodePosition( Edgeptr * edge_array, int num_edges );

/**
 * Sorts the array of nodes by increasing degree
 */
void sortByDegree( Nodeptr * node_array, int num_nodes );

/**
 * Updates the position field of each node on the layer to reflect the current
 * position in the nodes array.
 */
void updateNodePositions( int layer );

/**
 * Updates position fields for all nodes
 */
void updateAllPositions( void );

#endif
