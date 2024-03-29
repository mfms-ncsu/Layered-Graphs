/**
 * @file defs.h
 * @brief
 * data structures and global variables common to all parts of the program;
 * includes variables related to command-line options
 * @date 2008/12/19
 */

#ifndef DEFS_H
#define DEFS_H

#include<stdbool.h>

#include"order.h"

// parameters based on command-line options

/**
 * Maximum number of iterations for the main heuristic.
 * This is the number of the order of nodes on a layer is modified.
 * If neither max_iterations nor max_runtime is specified, standard_termination is used.
 */
extern int max_iterations;

/**
 * Maximum number of passes of the main heuristic.
 * Definition of a pass is heuristic-specific:
 *   - for layer sorting heuristics, a pass is when all layers have been sorted
 *   - for sifting heuristics, a pass is when all nodes have been sifted
 */
extern int max_passes;

/**
 * Time that the preprocessor (or heuristic if none) started running
 */
extern double start_time;

/**
 * Time that the program has been running since the start of preprocessing.
 */
#define RUNTIME (getUserSeconds() - start_time)

/**
 * Runtime (in seconds) at which the main heuristic will be terminated; the
 * termination takes place at this runtime or at max_iterations, whichever
 * comes first.  If neither max_iterations nor max_runtime is specified,
 * standard_termination is used.
 */
extern double max_runtime;

/**
 * True if using the standard, "natural" stopping criterion for the iterative
 * heuristic, e.g., no improvement after a sweep for barycenter.
 */
extern bool standard_termination;

/**
 * True if there is a list of favored edges based on predecessors and
 * successors of a central node
 */
extern bool favored_edges;

/**
 * True if taking average of averages when calculating barycenter or median
 * weights wrt both neighboring layers.  False if dividing total position by
 * total degree.
 */
extern bool balanced_weight;

extern char * heuristic;
extern char * preprocessor;

/**
 * minimization objective, currently used to determine sgf output, if any;
 * @todo use this to determine what information to track while the
 * heuristic is running
 *  "t" = total, "b" = bottleneck, "s" and "bs" for stretch and
 *  bottleneck stretch
 */
extern char * objective;

/**
 * structure to save layer orderings for minimum crossings so far
 */
extern Orderptr best_crossings_order;
/**
 * structure to save layer orderings for minimum edge crossings so far
 */
extern Orderptr best_edge_crossings_order;
/**
 * structure to save layer orderings for minimum total edge stretch so far
 */
extern Orderptr best_total_stretch_order;
/**
 * structure to save layer orderings for minimum bottleneck edge stretch so far
 */
extern Orderptr best_bottleneck_stretch_order;
/**
 * structure to save layer orderings for minimum crossings involving favored
 * edges so far
 */
extern Orderptr best_favored_crossings_order;

/**
 * True if the edge list (node list) is to be randomized after each pass of
 * mce (sifting)
 */
extern bool randomize_order;

/**
 * For barycenter heuristic: how to deal with nodes that have no
 *  edges in the direction on which weights are based: see
 *  adjust_weights_left() and adjust_weights_avg() in barycenter.c. LEFT is
 *  the default (the nodes follow their left neighbor; this keeps the nodes
 *  together and makes the heuristic more stable).
 */
extern enum adjust_weights_enum { NONE, LEFT, AVG } adjust_weights;

/**
 * Based on Matuszewski et al. "Extending sifting for k-layer straightline
 * crossing minimaization": The order in which nodes are sifted can be (1)
 * based on a layer-by-layer sweep; (2) based on their degree (largest degree
 * first); or (3) random.  Number (2), DEGREE, is the default and the only
 * option currently implemented. 
 */
extern enum sift_option_enum { LAYER, DEGREE, RANDOM } sift_option;

/**
 * When a node is sifted during sifting, mcn, or mce, one can either base its
 * position on the minimum number of total crossings or, as in the original
 * mce design, on (local) maximum number of crossings for an edge. These two
 * options are denoted by TOTAL and MAX, respectively. DEFAULT means use
 * TOTAL for sifting and mcn, MAX for mce.
 *
 * @todo The introduction of mce_s as a separate heuristic makes this enum
 * superfluous for now, but maybe it should be revived for the sake of
 * symmetry and completeness -- so that the sifting heuristic can be used
 * with bottleneck minimization
 */
extern enum sifting_style_enum { DEFAULT, TOTAL, MAX } sifting_style;

/**
 * During a pass of maximum crossings edge, each iteration fixes both an edge
 * and the two endpoints of the edge. A pass can end in one of three ways:
 * - all nodes are fixed (NODES); each node is sifted only once
 * - all edges are fixed (EDGES); both endpoints of an edge are sifted at
 * each iteration (fixing of nodes is irrelevant)
 * - as soon as both endpoints of the current edge are fixed (EARLY)
 * NODES appears to work best.
 * The new option, ONE_NODE, sifts only one endpoint of the max crossings
 * edge, the one with the most node crossings; does not appear to work very
 * well.
 */
extern enum mce_option_enum { NODES, EDGES, EARLY, ONE_NODE } mce_option;

/**
 * For Pareto optimization we can choose a variety of different objectives;
 * for now we consider two at a time. This option currently affects only what
 * gets updated and reported, not the behavior of any heuristic.
 *  NO_PARETO = no Pareto optimization, i.e., don't report Pareto points
 *  BOTTLENECK_TOTAL = maxEdgeCrossings(),numberOfCrossings()
 *  STRETCH_TOTAL = totalStretch(),numberOfCrossings()
 *  BOTTLENECK_STRETCH = maxEdgeCrossings(),totalStretch()
 */
extern enum pareto_objective_enum
 { NO_PARETO, BOTTLENECK_TOTAL, STRETCH_TOTAL, BOTTLENECK_STRETCH } pareto_objective; 

/**
 * Save the order at the end of the given iteration in a file called
 * capture-x.ord, where x is the iteration number. If the value is negative,
 * no capture takes place.
 */
extern int capture_iteration;

/**
 * true if one or more files representing best values of objective or
 * different stages of the run should be created;
 * the type/format of the file is determined by the type of the input file
 */
extern bool write_files;
/**
 * true if output should go to an ord file
 */
extern bool write_ord_output;
/**
 * true if output should go to an sgf file
 */
extern bool write_sgf_output;

/**
 * true if (sfg) output should be written to stdout,
 *  i.e., user specified the '-o OBJECTIVE' option to get the sgf format
 *  representing best value of the OBJECTIVE to go to stdout
 */
extern bool write_stdout;

/**
 * Output file names are of the form output_base_name-x.ord, where x is
 * information about the heuristic used.
 * If the input is an sgf file, output will be output_base_name-x.sgf.
 * The output_base_name is specified via the -w option
 * with the special case -w _ meaning the graph_name will be used
 */
extern char * output_base_name;

/**
 * True if verbose information about the graph should be printed
 */
extern bool verbose;

/**
 * -1 means no tracing, 0 means end of iteration only, trace_freq > 0 means
 *    print a trace message every trace_freq iterations.
 */
extern int trace_freq;

#endif
