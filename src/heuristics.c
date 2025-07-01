/**
 * @file heuristics.c
 * @author Matthias Stallmann
 * @date 2023-05-29
 * 
 * @copyright Copyright (c) 2023
 */

/**
 * Heuristics are responsible for the following
 * - increment pass at the beginning of each pass
 * - call end_of_pass() at the end
 * - increment iteration at the beginning of each iteration
 * - call end_of_iteration() at the end
 * - check termination_criterion_met when appropriate
 * - updating information related to all objectives
 * At the end of each iteration a heuristic is also responsible for ensuring
 * - the layer_index (sequence number) of each node corresponds to the index in the node array
 * - nodes are sorted by horizontal_position
 * - if the heuristic is not a verticality heuristic,
 *     the horizontal_position's of the nodes should minimize nonverticality for their order
 */

#include<stdio.h>
#include<stdlib.h>
#include<limits.h>
#include<string.h>
#include<math.h>

#include"defs.h"
#include"heuristics.h"
#include"graph.h"
#include"barycenter.h"
#include"median.h"
#include"dfs.h"
#include"gbfs.h"
#include"crossings.h"
#include"channel.h"
#include"sorting.h"
#include"graph_io.h"
#include"sifting.h"
#include"stats.h"
#include"swap.h"
#include"timing.h"
#include"random.h"
#include"verticality.h"
#include"dynamic_programming.h"

/**
 * if trace_freq is <= TRACE_FREQ_THRESHOLD, then a message is printed at the
 * end of each pass; end of pass messages don't appear otherwise.
 */
#define TRACE_FREQ_THRESHOLD 2

int iteration = 0;
int pass = 0;
int post_processing_iteration = -1;

int min_crossings = INT_MAX;
int post_processing_crossings = INT_MAX;
int min_edge_crossings = INT_MAX;
int min_crossings_iteration = -1;
int min_edge_crossings_iteration = -1;

/**
 * @brief true if max_iterations, max_passes, or max_runtime reached
 */
bool termination_criterion_met = false;

/**
 * buffer for formatting all tracePrint strings
 */
static char buffer[MAX_NAME_LENGTH];

#if ! defined( TEST )

void createDotFileName( char * output_file_name, const char * appendix )
{
  strcpy( output_file_name, graph_name );
  if ( strcmp( appendix, "" ) != 0 )
    strcat( output_file_name, "-" );
  strcat( output_file_name, appendix );
  strcat( output_file_name, ".dot" );
}

/**
 * Does the actual printing for tracePrint
 * @todo update this to include verticality
 */
static void trace_printer(int layer, const char * message) {
  int current_number_of_crossings = numberOfCrossings();
  int current_bottleneck_crossings = maxEdgeCrossings();
  double current_total_stretch = totalStretch();
  long current_nonverticality = getTotalNonverticality();
  char * tag = layer < 0 ? "+" : "";
  printf( "%siter %4d | ps %3d | lay %2d | xings %3d %3d*"
          " | bn %2d %2d* | str %5.2f %5.2f*"
          " | nv %3ld %3ld*"
          " | time %4.2f"
          " | %s\n",
          tag, iteration, pass, layer, current_number_of_crossings, total_crossings.best,
          current_bottleneck_crossings, bottleneck_crossings.best,
          current_total_stretch, total_stretch.best,
          current_nonverticality, total_nonverticality.best,
          RUNTIME, message);
}

void tracePrint( int layer, const char * message ) {
  static int previous_print_iteration = 0;
  if ( trace_freq > 0 && iteration % trace_freq == 0 
       && iteration > previous_print_iteration ) {
    trace_printer( layer, message );
    if ( layer >= 0 )
      previous_print_iteration = iteration;
  }
  else if ( trace_freq >= 0 && trace_freq <= TRACE_FREQ_THRESHOLD && layer < 0 ) {
      trace_printer( layer, message );
  }
}

static void improvementPrint(void) {
  if ( trace_freq < 0 ) {
    fprintf(stderr, "+++ pass: %d, iteration: %d, total: %d, bottleneck: %d,"
                    " vertical: %ld +++\n",
                    pass, iteration,
                    total_crossings.best, bottleneck_crossings.best, total_nonverticality.best);
  }
}

// /**
//  * @return true if any of the measures of interest have improved
//  * since the last call to this function
//  * Currently used only for the swapping that takes place during post processing
//  *  and apparently not needed there
//  */
static bool check_improvement( void )
{
  // avoid shortcut logic to make sure side effects really happen
  bool better_total_crossings = has_improved_int( & total_crossings );
  bool better_bottleneck_crossings = has_improved_int( & bottleneck_crossings );
  bool better_max_nonverticality = has_improved_long( & total_nonverticality );
  bool better_total_stretch = has_improved_double( & total_stretch );
  bool better_bottleneck_stretch = has_improved_double( & bottleneck_stretch );
  bool improvement =
    better_total_crossings
    || better_bottleneck_crossings
    || better_max_nonverticality
    || better_total_stretch
    || better_bottleneck_stretch
  ;
  if ( improvement ) improvementPrint();
  return improvement;
}

void end_of_iteration(void) {
#ifdef DEBUG
  printf( "-> end_of_iteration: iteration = %d\n", iteration);
#endif
  update_best_all();
#ifdef DEBUG
  fprintf(stderr, "### pass: %d, iteration: %d, total: %d, bottleneck: %d,"
                  " vertical: %ld, runtime: %5.2f\n",
  pass, iteration,
  total_crossings.best, bottleneck_crossings.best, total_nonverticality.best,
  RUNTIME);
#endif
  if ( iteration >= max_iterations || RUNTIME >= max_runtime ) {
      termination_criterion_met = true;
  }
#ifdef DEBUG
  printf( "<- end_of_iteration: iteration = %d, max_iterations = %d, done = %d\n",
          iteration, max_iterations, termination_criterion_met);
#endif
}

/**
 * Called at the end of a pass.
 * Increments number of passes and sets termination_criterion_met if max_passes is achieved 
 */
static void end_of_pass() {
#ifdef DEBUG
  fprintf(stderr, "-> end_of_pass, pass = %d, max_passes = %d\n", pass, max_passes);
#endif
  if ( pass >= max_passes ) termination_criterion_met = true;
}

#endif // ! defined( TEST )

bool isFixedNode( Nodeptr node ) { return node->fixed; }
bool isFixedEdge( Edgeptr edge ) { return edge->fixed; }
bool isFixedLayer( int layer ) { return layers[layer]->fixed; }
void fixNode( Nodeptr node ) { node->fixed = true; }
void fixEdge( Edgeptr edge ) { edge->fixed = true; }
void fixLayer( int layer ) { layers[layer]->fixed = true; }

bool allNodesFixed( void ) {
    for ( int index = 0; index < number_of_nodes; index++ ) {
        Nodeptr node = master_node_list[index];
        if( ! isFixedNode( node ) ) return false;
    }
    return true;
}

void clearFixedNodes( void ) {
    for ( int index = 0; index < number_of_nodes; index++ ) {
        Nodeptr node = master_node_list[index];
        node->fixed = false;
    }
}

void clearFixedEdges( void ) {
    for ( int index = 0; index < number_of_edges; index++ ) {
        Edgeptr edge = master_edge_list[index];
        edge->fixed = false;
    }
}

/**
 * @return a random edge
 */
Edgeptr randomEdge(void) {
  int random_index = genrand_int31() % number_of_edges;
  Edgeptr random_edge = master_edge_list[random_index];
  return random_edge;
}

/**
 * @brief choose a random edge and sift both of its endpoints,
 *  i.e., put each of them in a random position
 * @return the sifted edge
 */
Edgeptr siftRandomEdge(void) {
  Edgeptr to_sift = randomEdge();
  iteration++;
  randomSift(to_sift->up_node);
  end_of_iteration();
  if ( ! termination_criterion_met ) {
    iteration++;
    randomSift(to_sift->down_node);
    end_of_iteration();
  } 
  return to_sift;
}

void clearFixedLayers( void ) {
    for( int layer = 0; layer < number_of_layers; layer++ ) {
        layers[ layer ]->fixed = false;
    }
}

int totalDegree( int layer ) {
  int total = 0;
  int position = 0;
  for( ; position < layers[ layer ]->number_of_nodes; position++ ) {
      Nodeptr node = layers[ layer ]->nodes[ position ];
      total += node->up_degree + node->down_degree;
    }
  return total;
}

int maxDegreeLayer( void ) {
  int max_deg_layer = -1;
  int max_deg = -1;
  int layer = 0;
  for( ; layer < number_of_layers; layer++ ) {
      int layer_degree = totalDegree( layer );
      if ( layer_degree > max_deg )
        {
          max_deg_layer = layer;
          max_deg = layer_degree;
        }
    }
  return max_deg_layer;
}

Nodeptr maxDegreeNode( void ) {
    int max_degree = 0;
    Nodeptr max_degree_node = NULL;
    for ( int index = 0; index < number_of_nodes; index++ ) {
        Nodeptr node = master_node_list[index];
        if ( DEGREE( node ) > max_degree ) {
              max_degree = DEGREE(node);
              max_degree_node = node;
        }
    }
    return max_degree_node;
}

// ******* The actual heuristics

// crossings_test requires the functions related to fixing nodes and layers
// but not the heuristics.
#if ! defined(TEST)

void median(void) {
  tracePrint( -1, "^^^ start median" );
  while ( ! termination_criterion_met ) {
      pass++;
      medianUpSweep(1);
      if ( termination_criterion_met ) return;
      medianDownSweep(number_of_layers - 2);
      if ( termination_criterion_met ) return;
      end_of_pass();
      tracePrint( -1, "--- median end of pass" );
  }
}

void barycenter(void) {
  tracePrint( -1, "^^^ start barycenter" );
  while ( ! termination_criterion_met ) {
      pass++;
      if ( do_random_sift ) {
        siftRandomEdge();
        end_of_iteration();
        if ( termination_criterion_met ) break;
      }
      if ( do_random_sift ) siftRandomEdge();
      barycenterUpSweep(1);
      if ( termination_criterion_met ) return;
      barycenterDownSweep(number_of_layers - 2);
      if ( termination_criterion_met ) return;
      end_of_pass();
      tracePrint( -1, "--- bary end of pass" );
    }
}

void modifiedBarycenter( void )
{
  tracePrint( -1, "^^^ start modified barycenter" );
  while ( ! termination_criterion_met ) {
      clearFixedLayers();
       /* quit when all layers are fixed */
      while ( ! termination_criterion_met ) {
          pass++;
          if ( do_random_sift ) {
            iteration++;
            siftRandomEdge();
            end_of_iteration();
            if ( termination_criterion_met ) break;
          }
          // find un-fixed layer with maximum crossings
          int layer = maxCrossingsLayer();
          // -1 indicates none found
          if ( layer == -1 ) break;
          fixLayer( layer );

          iteration++;
          barycenterWeights(layer, BOTH);
          layerSort(layer);
          updateCrossingsForLayer(layer);
          optimizeLayerVerticality(layer, BOTH);
          end_of_iteration();
          if ( termination_criterion_met ) break;

          tracePrint( layer, "max crossings layer" );
          // stop if end_of_iteration() reports that max has been reached,
          // either directly or via the sweep functions
          barycenterUpSweep(layer + 1);
          if ( termination_criterion_met ) break;
          barycenterDownSweep(layer - 1);
          if ( termination_criterion_met ) break;
          end_of_pass();
          tracePrint( -1, "--- mod_bary end of pass" );
        }
      tracePrint( -1, "=== mod_bary, all layers fixed" );
    }
}

void verticalityBarycenter( void ) {
  centerAllLayers();
	tracePrint( -1, "^^^ start verticality barycenter" );
  while ( ! termination_criterion_met ) {
		clearFixedLayers();
		while ( ! termination_criterion_met ) {
      pass++;
      if ( do_random_sift ) {
        iteration++;
        siftRandomEdge();
        end_of_iteration();
        if ( termination_criterion_met ) break;
      }

			int layer = maxNonverticalityLayer();

			if ( layer == -1 ) break;
			fixLayer( layer );

      iteration++;
			barycenterWeights(layer, BOTH);
			layerSort(layer);
			optimizeLayerVerticality(layer, BOTH);
      updateCrossingsForLayer(layer);
      end_of_iteration();
      if ( termination_criterion_met ) break;

      tracePrint( layer, "max nonverticality layer" );
			barycenterUpSweep(layer + 1);
      if ( termination_criterion_met ) return;
			barycenterDownSweep(layer - 1);
      if ( termination_criterion_met ) return;
      end_of_pass();
      tracePrint( -1, "--- vertical_bary end of pass" );
		}
    tracePrint( -1, "=== vertical_bary, all layers fixed" );
	}
}

/****** Utility functions for sifting heuristics; each handles an iteration ********/

/**
 * Handles sifting of a node and all related bookkeeping.
 * Here sifting is based on minimizing the total number of crossings.
 */
static void sift_iteration( Nodeptr node ) {
  iteration++;
  sift( node );
  fixNode( node );
  sprintf( buffer, "$$$ %s, node = %s", heuristic, node->name );
  tracePrint( node->layer, buffer );
  end_of_iteration();
}

/**
 * Handles sifting of both endpoints of an edge and all related bookkeeping.
 * Here sifting is based on minimizing the maximum number of crossings
 * among edges incident on the node being sifted.
 *
 * @return true if max iterations reached
 */
static void edge_sift_iteration( Edgeptr edge ) {
  // figure out which of the two nodes to sift (none, one, or both)
  bool sift_up_node = false;
  bool sift_down_node = false;
  if ( ! isFixedNode( edge->up_node ) ) {
    sift_up_node = true;
  }
  if ( ! isFixedNode( edge->down_node ) ) {
    sift_down_node = true;
  }
  if ( sift_up_node ) {
    iteration++;
    sift_node_for_edge_crossings( edge, edge->up_node );
    fixNode( edge->up_node );
    sprintf( buffer, "$$$ %s, node = %s, position = %d",
             heuristic, edge->up_node->name, edge->up_node->layer_index );
    tracePrint( edge->up_node->layer, buffer );
    end_of_iteration();
    if ( termination_criterion_met ) return;
  }

  if ( sift_down_node ) {
    iteration++;
    sift_node_for_edge_crossings( edge, edge->down_node );
    fixNode( edge->down_node );
    sprintf( buffer, "$$$ %s, node = %s, position = %d",
             heuristic, edge->down_node->name, edge->down_node->layer_index );
    tracePrint( edge->down_node->layer, buffer );
    end_of_iteration();
    if ( termination_criterion_met ) return;
  }
}

/**
 * Handles sifting of both endpoints of an edge and all related bookkeeping.
 * Here sifting is based on minimizing the maximum nonverticality
 * among edges incident on the node being sifted.
 *
 * @return true if max iterations reached
 */
static void vertical_edge_sift_iteration(Edgeptr edge) {
  // figure out which of the two nodes to sift (none, one, or both)
  bool sift_up_node = false;
  bool sift_down_node = false;
  if ( ! isFixedNode( edge->up_node ) ) {
    sift_up_node = true;
  }
  if ( ! isFixedNode( edge->down_node ) ) {
    sift_down_node = true;
  }
  if ( sift_up_node ) {
    iteration++;
    sift_node_for_nonverticality(edge->up_node);
    fixNode( edge->up_node );
    sprintf( buffer, "$$$ %s, node = %s, index = %d, position = %d",
             heuristic, edge->up_node->name,
             edge->up_node->layer_index,
             edge->up_node->horizontal_position );
    tracePrint( edge->up_node->layer, buffer );
    end_of_iteration();
    if ( termination_criterion_met ) return;
  }

  if ( sift_down_node ) {
    iteration++;
    sift_node_for_nonverticality(edge->down_node);
      fixNode( edge->down_node );
      sprintf( buffer, "$$$ %s, node = %s, index = %d, position = %d",
             heuristic, edge->down_node->name,
             edge->down_node->layer_index,
             edge->down_node->horizontal_position );
      tracePrint( edge->down_node->layer, buffer );
      end_of_iteration();
      if ( termination_criterion_met ) return;
  }
}

/**
 * @brief Positions (sifts) the node to minimize the total overall stretch
 */
static void total_stretch_sift_iteration(Nodeptr node) {
  iteration++;
  sift_node_for_total_stretch(node);
  fixNode(node);
  sprintf(buffer, "$$$ %s, node = %s, position = %d",
          heuristic, node->name, node->layer_index);
  tracePrint(node->layer, buffer);
  end_of_iteration();
}

/******* The sifting heuristics *********/

void maximumCrossingsNode(void) {
  tracePrint( -1, "^^^ start maximum crossings node" );
  while ( ! termination_criterion_met ) {
    pass++;
    if ( randomize_order ) {
      genrand_permute(master_node_list, number_of_nodes, sizeof(Nodeptr));
    }
    clearFixedNodes();
    if ( do_random_sift ) {
      Edgeptr sifted_edge =siftRandomEdge();
      fixNode(sifted_edge->up_node);
      fixNode(sifted_edge->down_node);
    }
    while ( ! termination_criterion_met ) {
      Nodeptr node = maxCrossingsNode();
      if ( node == NULL ) break;
      sift_iteration(node);
    }
    end_of_pass();
    tracePrint( -1, "$$$ mcn, end of pass, all nodes fixed" );
  }
}

void maximumCrossingsEdgeWithSifting( void ) {
  tracePrint( -1, "^^^ start maximum crossings edge with sifting" );
  while ( ! termination_criterion_met ) {
      pass++;
      clearFixedNodes();
      clearFixedEdges();
      if ( do_random_sift ) {
        Edgeptr sifted_edge = siftRandomEdge();
        fixEdge(sifted_edge);
        fixNode(sifted_edge->up_node);
        fixNode(sifted_edge->down_node);
      }
      if ( randomize_order )
        genrand_permute(master_edge_list, number_of_edges, sizeof(Edgeptr));
      while ( ! termination_criterion_met ) {
        Edgeptr edge = maxCrossingsEdge();
        if ( edge == NULL || allNodesFixed() ) break;
        sprintf( buffer, "->- mce_t, edge %s -> %s",
                 edge->down_node->name, edge->up_node->name );
        tracePrint( edge->up_node->layer, buffer );
        if ( ! isFixedNode( edge->up_node ) ) {
          fixNode( edge->up_node );
          sift_iteration( edge->up_node );
          if ( termination_criterion_met ) break;
        }
        if ( ! isFixedNode( edge->down_node ) ) {
          fixNode( edge->down_node );
          sift_iteration(edge->down_node);
          if ( termination_criterion_met ) break;
        }
        fixEdge( edge );
      }
      end_of_pass();
      tracePrint( -1, "--- mce for total crossings, end pass" );
  }
}

void maximumCrossingsEdge( void ) {
  tracePrint( -1, "^^^ start maximum crossings edge" );
  while ( ! termination_criterion_met ) {
    pass++;
    clearFixedNodes();
    clearFixedEdges();
    if ( do_random_sift ) {
      Edgeptr sifted_edge = siftRandomEdge();
      fixEdge(sifted_edge);
      fixNode(sifted_edge->up_node);
      fixNode(sifted_edge->down_node);
    }
    if ( randomize_order )
      genrand_permute(master_edge_list, number_of_edges, sizeof(Edgeptr));
    while ( ! termination_criterion_met ) {
      Edgeptr edge = maxCrossingsEdge();
      if ( edge == NULL ) break;
      sprintf( buffer, "->- mce, edge %s -> %s",
               edge->down_node->name, edge->up_node->name );
      tracePrint( edge->up_node->layer, buffer );
      edge_sift_iteration(edge);
      fixEdge( edge );
    }
    end_of_pass();
    tracePrint( -1, "--- mce, end pass" );
  }
}

/**
 * @todo This is awkward since it's hard to find where this struct is initialized
 *       It's used in sift_node_for_nonverticality() in sifting.c to keep track of
 *       the best postion in a layer.     
 */
extern Positionptr best_verticality_positions;
Positionptr best_verticality_positions;

void maximumNonVerticalityEdge( void ) {
  best_verticality_positions = (Positionptr) calloc(1, sizeof(struct position_struct));
  init_position_struct(best_verticality_positions);
  tracePrint(-1, "^^^ start maximum nonverticality edge");
  centerAllLayers();
  while ( ! termination_criterion_met ) {
    pass++;
    clearFixedNodes();
    clearFixedEdges();
    if ( do_random_sift ) {
      Edgeptr sifted_edge = siftRandomEdge();
      fixEdge(sifted_edge);
      fixNode(sifted_edge->up_node);
      fixNode(sifted_edge->down_node);
    }
  	if ( randomize_order )
      genrand_permute(master_edge_list, number_of_edges, sizeof(Edgeptr));
      while ( ! termination_criterion_met ) {
      Edgeptr edge = maxNonverticalityEdge();
      if ( edge == NULL ) break;
      sprintf( buffer, "->- mnve, edge %s -> %s",
               edge->down_node->name, edge->up_node->name );
      tracePrint( edge->up_node->layer, buffer );
      vertical_edge_sift_iteration(edge);
      fixEdge(edge);
    }
    end_of_pass();
    tracePrint( -1, "--- mnve, end pass" );
  }
  cleanup_position_struct(best_verticality_positions);
  free(best_verticality_positions);
}

void maximumStretchEdge( void ) {
  tracePrint( -1, "^^^ start maximum strech edge with total stretch sifting" );
  while ( ! termination_criterion_met ) {
      pass++;
      clearFixedNodes();
      clearFixedEdges();
      if ( do_random_sift ) {
        Edgeptr sifted_edge = siftRandomEdge();
        fixEdge(sifted_edge);
        fixNode(sifted_edge->up_node);
        fixNode(sifted_edge->down_node);
      }
      while ( ! termination_criterion_met ) {
        Edgeptr edge = maxStretchEdge();
        if ( edge == NULL || allNodesFixed() ) break;
        sprintf( buffer, "->- mse, edge %s -> %s",
                 edge->down_node->name, edge->up_node->name );
        tracePrint( edge->up_node->layer, buffer );
        if ( ! isFixedNode( edge->up_node ) ) {
          total_stretch_sift_iteration( edge->up_node );
          fixNode( edge->up_node );
          if ( termination_criterion_met ) break;
        }
        if ( ! isFixedNode( edge->down_node ) ) {
          total_stretch_sift_iteration( edge->down_node );
          fixNode( edge->down_node );
          if ( termination_criterion_met ) break;
        }
        fixEdge( edge );
      }
      end_of_pass();
      tracePrint( -1, "--- mse with sifting, end pass" );
  }
}

// the value used in the Matuszewski et al. paper
#define MAX_FAILS 1

/**
 * Sifts node in decreasing order as determined by the input array
 * @return true if the number of crossings at the end of the pass improved upon those at the start
 */
static bool sift_decreasing(const Nodeptr * node_array,
                             int num_nodes, int initial_crossings) {
  pass++;
#ifdef DEBUG
  printf( "-> sift_decreasing, num_nodes = %d, crossings = %d\n",
          num_nodes, initial_crossings );
#endif
  // sift by decreasing 'weight' (degree in this case)
  for( int i = num_nodes - 1; i >= 0; i-- ) {
#ifdef DEBUG
      printf( "  sifting i = %d, node = %s\n", i, node_array[i]->name );
#endif
      iteration++;
      sift(node_array[i]);
      tracePrint( node_array[i]->layer, "^^^ sift_decreasing ^^^" );
      sprintf( buffer, " $$$ sift, node = %s, pos = %d",
               node_array[i]->name, node_array[i]->layer_index );
      tracePrint( node_array[i]->layer, buffer );
      end_of_iteration();
      if ( termination_criterion_met ) break;
  }
  end_of_pass();
  updateAllCrossings();
#ifdef DEBUG
  printf( "<- sift_decreasing, crossings = %d\n",
          numberOfCrossings() );
#endif
  return numberOfCrossings() < initial_crossings;
}
                                       
/**
 * Sifts node in increasing order as determined by the input array
 * @return false if the sift was unsuccessful, i.e., it did not improve upon
 * initial_crossings or if the maximum number of iterations was reached
 *
 * @note here the key is improvement upon the number of crossings at the
 * beginning of this sifting pass, not necessarily the number of crossings
 * overall.
 */
static bool sift_increasing(const Nodeptr * node_array,
                             int num_nodes, int initial_crossings) {
  pass++;
#ifdef DEBUG
  printf( "-> sift_increasing, num_nodes = %d, crossings = %d\n",
          num_nodes, initial_crossings );
#endif
  // sift by decreasing 'weight' (degree in this case)
  for( int i = 0; i < num_nodes; i++ ) {
#ifdef DEBUG
      printf( "  sifting i = %d, node = %s\n", i, node_array[i]->name );
#endif
      iteration++;
      sift(node_array[i]);
      tracePrint( node_array[i]->layer, "^^^ sift_increasing ^^^" );
      sprintf( buffer, " $$$ sift, node = %s, pos = %d",
               node_array[i]->name, node_array[i]->layer_index );
      tracePrint( node_array[i]->layer, buffer );
      end_of_iteration();
      if ( termination_criterion_met ) break;
  }
  end_of_pass();
  updateAllCrossings();
#ifdef DEBUG
  printf( "<- sift_decreasing, crossings = %d\n",
          numberOfCrossings() );
#endif
  return numberOfCrossings() < initial_crossings;
}
                                       
void sifting(void) {
  // sort nodes by increasing degree (other options not implemented yet); but
  // if randomize_order is true, then the order is randomized and the node
  // list is resorted before each pass
  sortByDegree(master_node_list, number_of_nodes);

  /* the sifting algorithm from the Matuszewski et al. paper, except that a
   * fixed number of iterations or passes or a specific runtime limit
   * is used instead of the standard stopping criterion
   */
  while( ! termination_criterion_met ) {
    if ( do_random_sift ) siftRandomEdge();
    updateAllCrossings();
    int crossings_before = numberOfCrossings();
    bool fail = false;
    if ( randomize_order ) {
      genrand_permute(master_node_list, number_of_nodes, sizeof(Nodeptr));
      sortByDegree(master_node_list, number_of_nodes);
    }
    fail = ! sift_decreasing(master_node_list, number_of_nodes, crossings_before);
    end_of_pass();
    tracePrint( -1, "--- end of sifting pass" );
    if ( termination_criterion_met ) break;
    if( fail ) {
      if ( randomize_order ) {
        genrand_permute(master_node_list, number_of_nodes, sizeof(Nodeptr));
        sortByDegree(master_node_list, number_of_nodes);
      }
      fail = ! sift_increasing( master_node_list, number_of_nodes,
                                crossings_before );
      end_of_pass();
      tracePrint( -1, "--- end of sifting pass" );
      if ( termination_criterion_met ) break;
    }
    else {
      if ( randomize_order ) {
        genrand_permute(master_node_list, number_of_nodes, sizeof(Nodeptr));
        sortByDegree(master_node_list, number_of_nodes);
      }
      fail = ! sift_decreasing( master_node_list, number_of_nodes,
                                crossings_before );
      end_of_pass();
      tracePrint( -1, "--- end of sifting pass" );
      if ( termination_criterion_met ) break;
    }
  }
}

// preprocessors

void guidedBreadthFirstSearch(void) {
  centerAllLayers();
  assignGbfsWeights();
  for ( int layer = 0; layer < number_of_layers; layer++ ) {
    layerSort(layer);
    updateCrossingsForLayer(layer);
  }
  adjustVerticalities();
}

void depthFirstSearch( void ) {
  centerAllLayers();
  assignDfsWeights();
  for ( int layer = 0; layer < number_of_layers; layer++ ) {
    layerSort(layer);
    updateCrossingsForLayer(layer);
  }
  adjustVerticalities();
}

/**
 * Assigns weights so that, in a subsequent layer sort, the last node on the
 * layer is moved to the middle position, the next to last on one side, the
 * third from last on the other, etc.
 */
static void weight_first_to_middle( int layer )
{
  int n = layers[ layer ]->number_of_nodes;
  int position = 0;
  for( ; position < n; position++ )
    {
      int position_from_last = n - position - 1;
      Nodeptr node = layers[ layer ]->nodes[ position ];
      node->weight
        = ( position_from_last % 2 == 0 )
        ? n / 2 - position_from_last
        : n / 2 + position_from_last;
    }
}

void middleDegreeSort( void )
{
  for ( int layer = 0; layer < number_of_layers; layer++ )
    {
      sortByDegree( layers[layer]->nodes, layers[layer]->number_of_nodes );
      weight_first_to_middle( layer );
      layerSort( layer );
    }
}

// The following is the original implementation - not of much use in a
// parallel setting because of the barycenter sweeps

/* void middleDegreeSort( void ) */
/* { */
/*   // sort nodes on the layer of the largest degree node so that it is in */
/*   // the middle and degree decreases as you move to the outside */
/*   Nodeptr node = maxDegreeNode(); */
/*   int layer = node->layer; */
/* #ifdef DEBUG */
/*   printf( "-> middleDegreeSort: node = %s, layer = %d\n", */
/*           node->name, layer ); */
/* #endif */
/*   sortByDegree( layers[layer]->nodes, layers[layer]->number_of_nodes ); */
/*   weight_first_to_middle( layer ); */
/*   layerSort( layer ); */

/*   // do barycenter up and down sweeps from the (sorted) max degree layer */
/* #ifdef DEBUG */
/*   printf( " middleDegreeSort: start upsweep\n" ); */
/* #endif */
/*   barycenterUpSweep( layer + 1 ); */
/* #ifdef DEBUG */
/*   printf( " middleDegreeSort: start downsweep\n" ); */
/* #endif */
/*   barycenterDownSweep( layer - 1 ); */
/* #ifdef DEBUG */
/*   printf( "<- middleDegreeSort\n" ); */
/* #endif */
/* } */

/**
 * Iterates left to right over nodes of the layer,
 * swapping each node with its successor if the swap reduces crossing number 
 * @return true if any swap has reduced the number of crossings
 */
static bool swapping_iteration(int layer) {
  updateAllCrossings();
#ifdef DEBUG
  int total_crossings = numberOfCrossings();
  fprintf(stderr, "-> swapping_iteration, layer = %d, crossings = %d\n",
          layer, total_crossings);
#endif
  post_processing_iteration++;
  bool improved = false;
  Layerptr layer_ptr = layers[layer];
  Nodeptr * nodes = layer_ptr->nodes;
  int layer_size = layer_ptr->number_of_nodes;
  for ( int i = 0; i < layer_size - 1; i++ ) {
    updateCrossingsForLayer(layer);
    int pre_swap_crossings = numberOfCrossingsLayer(layer);
    fprintf(stderr, "swapping %d and %d\n", i, i + 1);
    swap_nodes(layer, i, i + 1);
    updateAllCrossings();
//    updateCrossingsForLayer(layer);
    int post_swap_crossings = numberOfCrossingsLayer(layer);
    if ( post_swap_crossings >= pre_swap_crossings ) {
      // undo swap - it has not improved anything
      swap_nodes(layer, i + 1, i);
      updateAllCrossings();
    }
    else {
      improved = true;
#ifdef DEBUG
      fprintf(stderr, "  <-> swapped nodes %d and %d, crossings = %d\n",
              i, i + 1, numberOfCrossings());
#endif
    }
#ifdef DEBUG
    fprintf(stderr, "  before swap = %d, after = %d\n",
                    pre_swap_crossings, post_swap_crossings);
#endif
  }
  // updateCrossingsForLayer(layer);
  updateAllCrossings();
  update_best_all();
  tracePrint(layer, "<-> swapping");
#ifdef DEBUG
  printf( "<- swapping_iteration, crossings = %d\n",
          numberOfCrossings() );
#endif
  return improved;
}

/**
 * @todo
 * !!! This heuristic has a bug, exemplified by Instances/bug.sgf when run with
 *     the arguments in the comment at the top.
 *     The procedure works correctly (I think) if crossings are the only objective.
 *     Not a high priority at this point.
 *     option in experiments for now without much degredation in performance.
 * !!!
 */
void swapping( void ) {
  bool improved = true;
  post_processing_iteration = 0;
  updateAllCrossings();

#ifdef DEBUG
  printf( "-> swapping, total crossings = %d\n", numberOfCrossings() );
#endif

  tracePrint( -1, "<-> start swapping" );
  while ( improved ) {
    improved = false;
    for ( int layer = 0; layer < number_of_layers - 1; layer ++ ) {
      bool improved_iteration = swapping_iteration(layer);
      improved = improved || improved_iteration;
    }
    tracePrint( -1, "-- end of swapping iteration" );
  } // while improved
  updateAllCrossings();

#ifdef DEBUG
  printf( "<- swapping, total crossings = %d\n", numberOfCrossings() );
#endif
}

#endif // ! defined(TEST)
