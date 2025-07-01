/**
 * @file gbfs.h
 * @brief interface for function that assigns weights based on guided breadth-first search
 *        see the 2001 JEA paper by Stallmann et al.
 * @author Matthias Stallmann
 * @date 2025-01-28
 */

#ifndef GBFS_H
#define GBFS_H

/**
 * Assigns weights to nodes based on their ordering in the final bfs
 * of the gbfs heuristic
 */
void assignGbfsWeights(void);

#endif