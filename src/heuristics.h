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
#include"positions.h"

/**
 * The current iteration, or, the number of iterations up to this point.
 * Needs to be extern for barycenter variants and median only
 */
extern int iteration;

/**
 * The current pass, or, the number of pass up to this point.
 * Needs to be extern for debugging convenience
 */
extern int pass;

/**
 * @brief true if max_iterations, max_passes, or max_runtime is reached
 * Needs to be extern for barycenter variants and median only
 */
extern bool termination_criterion_met;

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
 * Does things that are appropriate at the end of an iteration, such as
 * checking whether minimum values and configurations for various objectives need updating
 * Also sets termination_criterion_met
 * if the specified max_iterations or max_runtime have been reached  
 * Needs to be extern for barycenter variants and median only
 */
void end_of_iteration(void);

/**
 * Prints information about current number of iterations, crossings, etc.,
 * @param message a message identifying the context of the printout.
 * @param layer the layer that was just sorted
 */
void tracePrint( int layer, const char * message );

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
 * @return a random edge that has not been fixed or NULL if none exists
 */
Edgeptr randomEdge(void);

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
 *  - find an unmarked layer k for which incident edges have the most crossings and mark it
 *  - sort layer k based on barycenter weights of both the upper and lower neighbors
 *  - subsequent iterations sort
 *    + layers k-1 to 0 based on upper neighbors
 *    + layers k+1 to L-1 based on lower neighbors (L = # of layers)
 * Do this repeatedly, each repetition is a pass
 */
void modifiedBarycenter( void );

/**
 * @todo
 * Create another verticality barycenter option based on classic barycenter sweeps
 * and rename the current on to something like modifiedVerticalityBarycenter.
 * Not high priority
 */

/**
 * @brief 
 * A variation on modified barycenter that works as follows:
 * - find an unmarked layer k whose incident edges have maximum total nonverticality and mark it
 * - sort layer k based on average *position*, as opposed to index, of neighbors above and below
 * - run a dynamic programming algorithm that minimizes nonverticality
 *   given a fixed sequence of nodes
 * - analogous to modifiedBarycenter sort and apply the DP algorithm to
 *   + layers k-1 to 0 based on upper neighbors
 *   + layers k+1 to L-1 based on lower neighbors
 */
void verticalityBarycenter( void );

/**
 * @brief A variation of mce focused on minimizing nonverticality.
 */
void maximumNonVerticalityEdge( void );

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
 * 
 * @warning this currently does not work;
 *           crashes on u_50_40_105_1-rnd-009-scr
 */
void maximumStretchEdge( void );

/* the sifting algorithm from the Matuszewski et al. paper, except that a
* fixed number of iterations or passes or a specific runtime limit
* is used instead of the standard stopping criterion
*/
void sifting( void );

/**
 * @todo Not yet implemented; initially the two objectives will be crossings and nonverticality,
 *       but can be adapted to any pair of objectives.
 *       Not clear whether it makes more sense to have well-defined passess
 *        with each node chosen once per pass or to make the choice completely random
 *       The former fits better into the overall scheme, and, if no random seed is provided,
 *        can select nodes by decreasing degree.
 * @brief Each iteration of this algorithm works as follows
 * - record current number of crossings cx and the total nonverticality nv
 * - choose a random node x
 * - sift x with respect to crossings and record diff_cx_cx and diff_cx_nv,
 *     the differences in number of crossings and noverticality; also save the config as CX
 * - sift x with respect to nonverticality and record diff_nv_cx and diff_nv_nv,
 *     the differences in number of crossings and nonverticality; also save the config as NV
 * Now there are four diffs, each of which can be negative (better) or positive (worse);
 * assign -1 for better and +1 for worse and compare the sum of the two for each type of sifting.
 * If diff_cx_sum < diff_nv_sum, the next config is CX
 * If diff_nv_sum < diff_cx_sum, the next config is NV
 * Otherwise choose the config at random. 
 */
void randomWalk(void);

// preprocessors

void guidedBreadthFirstSearch( void );

void depthFirstSearch( void );

void middleDegreeSort( void );

// post processing

/**
 * Swaps neighboring nodes on each layer when this improves the total number of crossings
 * until no improvement has occurred during a pass.
 */
void swapping(void);

#endif
