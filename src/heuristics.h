/**
 * @file heuristics.h
 * @brief Interface for functions implementing all of the heuristics
 *
 * Every heuristic is responsible for maintaining the following two invariants.
 *  - for every node, node->position is correct after each iteration
 *  - the array of nodes on each layer is sorted by increasing position
 *
 * @author Matthias Stallmann
 */

#ifndef HEURISTICS_H
#define HEURISTICS_H

#include<stdbool.h>
#include"graph.h"
#include"order.h"

/**
 * The current iteration, or, the number of iterations up to this point.
 */
extern int iteration;

/**
 * The minimum total number of crossings during post processing
 */
extern int post_processing_crossings;

/**
 * The current iteration during post processing
 */
extern int post_processing_iteration;

/**
 * Creates a dot file name using the graph name and the appendix
 * @param output_file_name a buffer for the file name to be created, assumed
 * to be big enough
 * @param appendix a string that is attached just before the .dot extension
 */
void createDotFileName( char * output_file_name, const char * appendix );

/**
 * Prints information about current number of iterations, crossings, etc.,
 * @param message a message identifying the context of the printout.
 * @param layer the layer that was just sorted
 */
void tracePrint( int layer, const char * message );

/**
 * Does things that are appropriate at the end of an iteration, such as
 * checking whether the ordering needs to be captured and the min
 * crossings/edge crossings need to be updated. Also increments the
 * iteration counter
 *
 * @return true if max_iterations has been reached
 */
bool end_of_iteration( void );

// ******** maintenance of fixed nodes and layers (for many of the
// ******** heuristics)

bool isFixedNode( Nodeptr node );
bool isFixedEdge( Edgeptr edge );
bool isFixedLayer( int layer );
void fixNode( Nodeptr node );
void fixEdge( Edgeptr edge );
void fixLayer( int layer );
void clearFixedNodes( void );
void clearFixedEdges( void );
void clearFixedLayers( void );

// ******** Miscellaneous

/**
 * @return the total degree of nodes on the given layer
 */
int totalDegree( int layer );

/**
 * @return the layer with maximum total degree
 */
int maxDegreeLayer();

// ******** The actual heuristics 

/**
 * implements the median heuristic
 */
void median( void );

/**
 * implements the barycenter heuristic
 */
void barycenter( void );

/**
 * @brief Implements the modified barycenter heuristic, which goes as follows.
 * Repeat the following until all layers have been marked.
 *  - find a layer k for which incident edges have the most crossings and mark it
 *    layer k is sorted based on barycenter weights of both the upper and lower neighbors
 *  - subsequent iterations sort
 *    + layers k-1 to 0 based on upper neighbor
 *    + layers k+1 to L-1 based on lower neighbor (L = # of layers)
 * Do this repeatedly, each repetition is a pass
 */
void modifiedBarycenter( void );

// !!! The following heuristics, designed for parallel implementation,
// !!! are deprecated; they are ineffective

/**
 * !!! Computes weights on each layer independently and alternates sweep directions
 * <em>(parallel)</em>
 */
void staticBarycenter( void );

/**
 * !!! Alternates barycenter iterations between even and odd layers, computing
 * weights for both adjoining layers each time.
 * <em>(parallel)</em>
 */
void evenOddBarycenter( void );

/**
 * !!! Alternates between odd and even numbered layers, but instead of
 * alternating at every iteration and using both neighboring layers to assign
 * weights, this version uses the downward layer (starting with odd layers)
 * for a number of iterations corresponding to the number of layers, and then
 * the upward layer for an equal number of iterations.
 * <em>(parallel)</em>
 */

/**
 * !!! Alternates between even numbered and odd numbered layers. Unlike alt_bary,
 * which bases sorting on both neighboring layers simultaneously, this
 * version rotates between doing upward, downward, and both.
 * <em>(parallel)</em>
 */
void rotatingBarycenter( void );

void upDownBarycenter( void );

/**
 * !!! Each processor does a full-blown barycenter algorithm. Starts are
 * staggered at distance max(layers/processors,2) and each sweep wraps around  
 * to make the best use of of each processor. Startting layer shifts by 1 at
 * each iteration.
 *
 * @todo rename this as shiftBarycenter, which would be more in keeping with
 * its behavior.
 */
void slabBarycenter( void );

// !!! END OF DEPRECATED HEURISTICS !!!

/**
 * mce as described in M. Stallmann, JEA 2012.
 * 
 * The variations described below modify two aspects of mce.
 * 1. Basis for choosing a node to sift - mce uses endpoint(s) of an edge with most crossings.
 * Could also use
 *    - node whose incident edges have most crossings
 *    - edge with largest stretch
 *    - edge with largest nonverticality
 *    - nodes whose incident edges have maximum stretch or nonverticality
 * 2. Objective to use for deciding where to place the sifted node x.
 *    - minimize maximum number of crossings among edges incident on x (mce)
 *    - minimize total crossings
 *    - minimize maximum stretch/nonverticality for edges incident on x
 *    - minimize total stretch/nonverticality
 * 
 * @todo could also consider minimizing total number of ___ for edges incident on x
 */
void maximumCrossingsEdge( void );

/**
 * @brief A variation of mce: instead of choosing the edges with maximum number
 *        of crossings each iteration and sifting the endpoints,
 *        finds the node whose incident edges have the maximum number of crossings
 *        and sift it
 */
void maximumCrossingsNode( void );

/**
 * A variation of the mce heuristic in which the two endpoints of the edge
 * with maximum crossings are sifted so as to minimize the total number of
 * crossings rather than the more complicated objective of mce that is
 * described in M. Stallmann, JEA 2012.
 */
void maximumCrossingsEdgeWithSifting( void );

/**
 * similar to mce, except that, in each iteration, the edge with maximum
 * stretch is chosen and the endpoints are moved to positions that minimize
 * the total stretch of their incident edges
 *
 * @todo one could also base the movement on minimizing the maximum stretch
 * of any edge, similar to mce
 */
void maximumStretchEdge( void );

void sifting( void );

// preprocessors

void breadthFirstSearch( void );

void depthFirstSearch( void );

void middleDegreeSort( void );

// post processing

/**
 * Swaps neighboring nodes when this improves the total number of crossings
 * until no improvement is possible.
 * <em>(embarrasingly parallel)</em>
 * 
 * @todo could do swapping based on any other objective
 */
void swapping( void );

#endif

/*  [Last modified: 2021 01 06 at 16:04:34 GMT] */
