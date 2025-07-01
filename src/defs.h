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
#include<stdio.h>

/**
 * @todo not clear why some definitions are in constants.h or why positions.h needs to be included:
 *       maybe put everything in one place
 */
#include"constants.h"
#include"positions.h"

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
 * True if taking average of averages when calculating barycenter or median
 * weights wrt both neighboring layers.  False if dividing total position by
 * total degree.
 */
extern bool balanced_weight;

extern char * heuristic;
extern char * preprocessor;

/**
 * @brief true if the context is a heuristic that optimizes verticality;
 *        this is not the same as having verticality as the objective;
 *        you can run a verticality heuristic and output the configuration with
 *        minimum crossings
 */
extern bool verticality_heuristic;

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
extern Positionptr best_crossings_order;
/**
 * structure to save layer orderings for minimum edge crossings so far
 */
extern Positionptr best_edge_crossings_order;
/**
 * structure to save layer orderings for minimum total edge stretch so far
 */
extern Positionptr best_total_stretch_order;
/**
 * structure to save layer orderings for minimum bottleneck edge stretch so far
 */
extern Positionptr best_bottleneck_stretch_order;
/**
 * structure to save layer orderings for minimum crossings involving favored
 * edges so far
 */
extern Positionptr best_favored_crossings_order;
/**
 * structure to save layer orderings for minimum nonverticality so far
 */
extern Positionptr best_nonverticality_order;
/**
 * structure to save layer orderings for minimum bottleneck verticality so far 
 */
extern Positionptr best_bottleneck_verticality_order;

/**
 * True if the edge list (node list) is to be randomized after each pass of
 * mce (sifting)
 */
extern bool randomize_order;

/**
 * If true, do a random sift of the endpoints of a random edge at the beginning of each pass.
 * A random sift puts the sifted node in a random position on its layer.
 * Currently implemented for verticality heurisitics.
 */
extern bool do_random_sift;

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
 * @todo The introduction of mce_t as a separate heuristic makes this enum
 * superfluous for now, but maybe it should be revived for the sake of
 * symmetry and completeness -- so that the sifting heuristic can be used
 * with bottleneck minimization
 */
extern enum sifting_style_enum { DEFAULT, TOTAL, MAX } sifting_style;

/**
 * For Pareto optimization we can choose a variety of different objectives;
 * for now we consider two at a time. This option currently affects only what
 * gets updated and reported, not the behavior of any heuristic.
 *  NO_PARETO = no Pareto optimization, i.e., don't report Pareto points
 *  BOTTLENECK_TOTAL = maxEdgeCrossings(), numberOfCrossings()
 *  TOTAL_STRETCH = numberOfCrossings(), totalStretch()
 *  BOTTLENECK_STRETCH = maxEdgeCrossings(), totalStretch()
 *  TOTAL_VERTICAL = numberOfCrossings(), updateAllVerticality()
 *  BOTTLENECK_VERTICAL = maxEdgeCrossings(), updateAllVerticality()
 */
extern enum pareto_objective_enum
 { NO_PARETO, BOTTLENECK_TOTAL, TOTAL_STRETCH, BOTTLENECK_STRETCH,
                                TOTAL_VERTICAL, BOTTLENECK_VERTICAL } pareto_objective;

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
 * True if verbose information about the graph should be printed
 */
extern bool verbose;

/**
 * -1 means no tracing, 0 means end of iteration only, trace_freq > 0 means
 *    print a trace message every trace_freq iterations.
 */
extern int trace_freq;

#endif
