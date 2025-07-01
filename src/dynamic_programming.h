/**
 * @file dynamic_programming.h
 * @author Matt Stallmann
 * @brief header for optimal dynamic programming algorithm to minimize nonverticality
 *        of a sorted layer
 * @date 2022-04-26
 */

#ifndef DYNAMIC_PROGRAMMING_H
#define DYNAMIC_PROGRAMMING_H

#include"graph.h"

/**
 * @brief dynamic programming algorithm to minimize nonverticality
 *        of the layer while maintaining the order of the nodes
 * @param layer array of nodes on the layer to be optimized
 * @param direction whether the cost function is to be based on verticality
 *                  of edges going upward, downward, or both
 */
void dynamicProgramming(Layerptr layer, Orientation direction);

/**
 * @brief allocates memory for the dynamic programming algorithm
 *        three matrices large enough to accomodate the largest layer
 */
void allocateDPMatrices(void);

/**
 * @brief deallocates memory used by the dynamic programming algorithm
 */
void deallocateDPMatrices(void);

#endif      // DYNAMIC_PROGRAMMING_H