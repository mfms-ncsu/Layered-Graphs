/**
 * @file verticality.h
 * @author Matthias Stallmann (mfms@ncsu.edu)
 * @brief declarations of functions related to maintaining verticality information
 * 
 * @copyright Copyright (c) 2022
 */


#ifndef VERTICALITY_H
#define VERTICALITY_H

#include "graph.h"
#include "dynamic_programming.h"

/**
 * @return the total nonverticality of all edges incident on the node
 * @pre an update has taken place since the last position change
 */
long getNodeNonverticality(Nodeptr node);

/**
 * @brief initializes the nonverticality of all edges in the graph
 */
void initializeVerticality(void);

/**
  * @return the total nonverticality of all edges
 */
long getTotalNonverticality(void);

/**
  * @return the maximum nonverticality among the edges
 */
int getBottleneckNonverticality(void);

/**
 * @brief updates nonverticality of all edges in the graph
 * @return the total nonverticality
 */
long updateAllVerticality(void);

/**
 * @brief updates nonverticality of edges incident on nodes of the layer
 * @return the total nonverticality of the layer
 */
long updateLayerVerticality(int layer_number);

/**
 * @brief updates nonverticality of edges incident on the node
 * @param node the node to be updated
 * @param direction whether the incident edges to be considered are upward, downward, or both
 * @return the total nonverticality of edges incident on the node
 */
long updateNodeVerticality(Nodeptr node, Orientation direction);

/**
 * @brief finds the layer with the maximum non-verticality
 * @return The number of an unfixed layer whose incident edges have the largest total non-verticality, or -1 if layers are fixed.
 */
int maxNonverticalityLayer( void );


/**
 * @brief finds the edge with the maximum non-verticality
 * @return The number of an unfixed edge with the largest non-verticality, or NULL if all edges are fixed.
 */
Edgeptr maxNonverticalityEdge( void );

/**
 * @brief sets positions of the nodes so that they are centered wrt maximum width
 * @param layer_number number of layer whose nodes should be centered
 */
void centerNodes(int layer_number);

/**
 * @brief optimizes the verticality of the layer using a dynamic programming algorithm
 *        and recalculates verticalities for nodes on the layer
 * 
 * @param layer_index index of the relevant layer
 * @param direction whether the optimization is to be based on verticalities
 * 					of edges going upward, downward or both
 */
void optimizeLayerVerticality(int layer_index, Orientation direction);

/**
 * @brief centers nodes on every layer with respect to maximum layer width
 */
void centerAllLayers(void);

/**
 * @brief uses centering and dynamic programming to find the best verticality positions
 * for all nodes without changing the order on any layer
 */
void adjustVerticalities(void);

#endif
