/**
 * @file heuristics.c
 * @brief High-level implementations of various heuristics
 * @author Matthias Stallmann
 * @date 2008/12/29
 * $Id: heuristics.c 140 2016-02-15 16:13:56Z mfms $
 */

/**
 * @todo The incrementing of 'iteration' is far from transparent; it takes
 * place in end_of_iteration(), which is called from a number of different
 * places, most in other modules.
 * The right way to do this is (probably):
 *   - get rid of the end_of_iteration() call in main.c
 *   - have each heuristic increment iteration at the *beginning* of an iteration
 *   - don't increment iteration in end_of_iteration()
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
#include"crossings.h"
#include"channel.h"
#include"sorting.h"
#include"graph_io.h"
#include"sifting.h"
#include"stats.h"
#include"swap.h"
#include"timing.h"
#include"random.h"

/**
 * if trace_freq is <= TRACE_FREQ_THRESHOLD, then a message is printed at the
 * end of each pass; end of pass messages don't appear otherwise.
 */
#define TRACE_FREQ_THRESHOLD 2

int iteration = 0;
int pass = 0;
int post_processing_iteration = 0;

int min_crossings = INT_MAX;
int post_processing_crossings = INT_MAX;
int min_edge_crossings = INT_MAX;
int min_crossings_iteration = -1;
int min_edge_crossings_iteration = -1;

/**
 * buffer for formatting all tracePrint strings
 */
static char buffer[ MAX_NAME_LENGTH ];


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
 *
 * @todo Use trace_freq == -1 to indicate that printing should only be done
 * if there has been improvement since last time; can use INT_MAX as default
 * instead of -1.
 *
 * The logic would be something like:
 *  - update crossings
 *  - trace_freq == -1 then
 *      _ update_best (stats.c)
 *      _ return if ! has_improved (stats.c)
 */
static void trace_printer( int layer, const char * message )
{
  updateAllCrossings();
  int number_of_crossings = numberOfCrossings();
  int bottleneck_crossings = maxEdgeCrossings();
  double current_total_stretch = totalStretch();
  char * tag = layer < 0 ? "+" : "";
  printf( "%siteration %4d | layer %2d | crossings %3d | best %3d"
          " | bottleneck %2d | best %2d | stretch %5.2f | best %5.2f"
          " | time %4.2f"
          " | %s\n",
          tag, iteration, layer, number_of_crossings, total_crossings.best,
          bottleneck_crossings, max_edge_crossings.best,
          current_total_stretch, total_stretch.best,
          RUNTIME, message );
}

void tracePrint( int layer, const char * message )
{
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

/**
 * @return true if none of the measures of interest have improved
 * since the last call to this function
 *
 * <em>Side effect:</em> all measures are updated
 * @todo should probably rethink this to focus on relevant measures
 */
static bool no_improvement( void )
{
  // avoid shortcut logic to make sure side effects really happen
  bool better_total_crossings = has_improved_int( & total_crossings );
  bool better_max_edge_crossings = has_improved_int( & max_edge_crossings );
  bool better_total_stretch = has_improved_double( & total_stretch );
  bool better_bottleneck_stretch = has_improved_double( & bottleneck_stretch );
  return
    ! better_total_crossings
    && ! better_max_edge_crossings
    && ! better_total_stretch
    && ! better_bottleneck_stretch
    ;
}

/**
 * Prints a message if number of crossings (of various types) are still
 * improving but max iterations have been reached.
 */
static void print_last_iteration_message( void )
{
  // stopping early because of reaching max iterations even though
  // improvement has occurred
  if ( iteration >= max_iterations 
       && ! no_improvement() )
    {
      /**
       * @todo more information in the "still improving" message
       */
        fprintf(stderr, "$$$ still improving but max iterations or runtime reached:"
              " iteration %d, runtime %2.3f, graph %s\n",
              iteration, RUNTIME, graph_name );
    }
}

bool end_of_iteration( void )
{
#ifdef DEBUG
  printf( "-> end_of_iteration: iteration = %d, capture_iteration = %d\n",
          iteration, capture_iteration );
#endif
  if( capture_iteration == iteration && write_files ) {
      char appendix[MAX_NAME_LENGTH];
      sprintf(appendix, "%d", iteration);
      writeFile(appendix);
  }
#ifdef DEBUG
  printf( " ### crossings at end of iteration %d:\n"
          "   current: total = %d,"
          " edge = %d"
          "\n"
          "   best:    total = %d,"
          " edge = %d"
          "\n"
          "          %d"
          "\n",
          iteration,
          numberOfCrossings(),
          maxEdgeCrossings(),
          total_crossings.best,
          max_edge_crossings.best,
          total_crossings.best_heuristic_iteration
          , max_edge_crossings.best_heuristic_iteration
          );
#endif // DEBUG
  bool done = false;
  update_best_all();
  if ( iteration >= max_iterations ||  RUNTIME >= max_runtime ) {
      done = true;
      print_last_iteration_message();
  }
  iteration++;
#ifdef DEBUG
  printf( "<- end_of_iteration: iteration = %d, max_iterations = %d, done = %d\n",
          iteration, max_iterations, done );
#endif
  return done;
}

/**
 * Prints a message to indicate the point at which standard termination would
 * have occurred if the option is to keep going. This is called at the end of
 * a pass. It will not be called if max iterations has been reached.
 */
static void print_standard_termination_message()
{
  /** message already printed? */
  static bool standard_termination_message_printed = false;

  if ( ! standard_termination_message_printed ) {
        fprintf(stderr, "=== standard termination here: iteration %d crossings %d"
              " bottleneck %d"
              " graph %s ===\n",
              iteration, total_crossings.best,
              max_edge_crossings.best,
              graph_name);
  }
  standard_termination_message_printed = true;
}

/**
 * Called at the end of a pass.
 * @return true if one of the following holds
 *          - # of iterations >= what user specified with the -i option
 *          - # of passes >= what user specified with the -a option
 *          - no improvement has occurred in any objective if neither -i nor -a specified
 * Prints a message about failure to improve even if stopping criterion is
 * number of iterations.
 */
static bool terminate()
{
  // no_improvement() has side effects
  bool no_improvement_seen
    = no_improvement();

  // separating message from actual termination
  if ( no_improvement_seen )
    print_standard_termination_message();

  if ( standard_termination && no_improvement_seen ) return true;
  if ( iteration >= max_iterations ) return true;
  if ( pass >= max_passes ) return true;
  pass++;
  return false;
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

void clearFixedLayers( void ) {
    for( int layer = 0; layer < number_of_layers; layer++ ) {
        layers[ layer ]->fixed = false;
    }
}

int totalDegree( int layer )
{
  int total = 0;
  int position = 0;
  for( ; position < layers[ layer ]->number_of_nodes; position++ )
    {
      Nodeptr node = layers[ layer ]->nodes[ position ];
      total += node->up_degree + node->down_degree;
    }
  return total;
}

int maxDegreeLayer( void )
{
  int max_deg_layer = -1;
  int max_deg = -1;
  int layer = 0;
  for( ; layer < number_of_layers; layer++ )
    {
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

void median( void )
{
  tracePrint( -1, "^^^ start median" );
  while( ! terminate() )
    {
      // return if max iterations have been reached as reported by one of the
      // sweep functions
      if ( medianUpSweep( 1 ) )
        return;
      if ( medianDownSweep( number_of_layers - 2 ) )
        return;
      tracePrint( -1, "--- median end of pass" );
    }
}

void barycenter( void )
{
  tracePrint( -1, "^^^ start barycenter" );
  while( ! terminate() )
    {
      // return if max iterations have been reached as reported by one of the
      // sweep functions
      if ( barycenterUpSweep( 1 ) )
        return;
      if ( barycenterDownSweep( number_of_layers - 2 ) )
        return;
      tracePrint( -1, "--- bary end of pass" );
    }
}

void modifiedBarycenter( void )
{
  tracePrint( -1, "^^^ start modified barycenter" );
  while( ! terminate() ) {
      clearFixedLayers();
      /* quit when all layers are fixed */
      while ( true ) {
          // find un-fixed layer with maximum crossings
          int layer = maxCrossingsLayer();
          // -1 indicates none found
          if ( layer == -1 ) break;
          fixLayer( layer );

          barycenterWeights( layer, BOTH );
          layerSort( layer );
          updateCrossingsForLayer( layer );

          tracePrint( layer, "max crossings layer" );
          // stop if end_of_iteration() reports that max has been reached,
          // either directly or via the sweep functions
          if ( end_of_iteration() )
            return;
          if ( barycenterUpSweep( layer + 1 ) )
            return;
          if ( barycenterDownSweep( layer - 1 ) )
            return;
          tracePrint( -1, "--- mod_bary end of pass" );
        }
      tracePrint( -1, "=== mod_bary, all layers fixed" );
    }
}

/**
 * Handles sifting of a node and all related bookkeeping.
 * Here sifting is based on minimizing the total number of crossings.
 * @return true if max_iterations reached
 */
static bool sift_iteration( Nodeptr node )
{
  sift( node );
  fixNode( node );
  sprintf( buffer, "$$$ %s, node = %s", heuristic, node->name );
  tracePrint( node->layer, buffer );
  if ( end_of_iteration() ) return true;
  return false;
}

/**
 * Handles sifting of both endpoints of an edge and all related bookkeeping.
 * Here sifting is based on minimizing the maximum number of crossings
 * among edges incident on the node being sifted.
 *
 * @return true if max iterations reached
 */
static bool edge_sift_iteration( Edgeptr edge )
{
  // figure out which of the two nodes to sift (none, one, or both)
  bool sift_up_node = false;
  bool sift_down_node = false;
  if ( mce_option == EDGES ) {
    sift_up_node = sift_down_node = true;
  }
  if ( ! isFixedNode( edge->up_node ) ) {
    sift_up_node = true;
  }
  if ( ! isFixedNode( edge->down_node ) ) {
    sift_down_node = true;
  }
  if ( mce_option == ONE_NODE ) {
    // if neither node is fixed, sift only the one with the most crossings
    if ( sift_up_node && sift_down_node ) {
      if ( numberOfCrossingsNode( edge->down_node )
           > numberOfCrossingsNode( edge->up_node ) ) {
        sift_up_node = false;
      }
      else {
        sift_down_node = false;
      }
    }
  }
  if ( sift_up_node )
    {
      sift_node_for_edge_crossings( edge, edge->up_node );
      fixNode( edge->up_node );
      sprintf( buffer, "$$$ %s, node = %s, position = %d",
               heuristic, edge->up_node->name, edge->up_node->position );
      tracePrint( edge->up_node->layer, buffer );
      if ( end_of_iteration() )
        return true;
    }

  if ( sift_down_node )
    {
      sift_node_for_edge_crossings( edge, edge->down_node );
      fixNode( edge->down_node );
      sprintf( buffer, "$$$ %s, node = %s, position = %d",
               heuristic, edge->down_node->name, edge->down_node->position );
      tracePrint( edge->down_node->layer, buffer );
      if ( end_of_iteration() )
        return true;
    }
  return false;
}

static bool total_stretch_sift_iteration( Nodeptr node ) {
  sift_node_for_total_stretch(node);
  fixNode(node);
  updateAllCrossings();
  sprintf(buffer, "$$$ %s, node = %s, position = %d",
          heuristic, node->name, node->position);
  tracePrint(node->layer, buffer);
  if (end_of_iteration())
    return true;
  return false;
}

void maximumCrossingsNode( void )
{
  tracePrint( -1, "^^^ start maximum crossings node" );
  while( ! terminate() )
    {
      clearFixedNodes();
      while ( true )
        // keep going until all nodes are fixed
        {
          Nodeptr node = maxCrossingsNode();
          if ( node == NULL ) break;
          bool last_iteration = false;
          /* if ( sifting_style == MAX ) */
          /*   last_iteration = edge_sift_iteration( edge, node ); */
          /* else */
            last_iteration = sift_iteration( node );
          if ( last_iteration )
            return;
        }
      tracePrint( -1, "$$$ mcn, all nodes fixed" );
    }
}

void maximumCrossingsEdgeWithSifting( void ) {
  tracePrint( -1, "^^^ start maximum crossings edge with sifting" );
  while( ! terminate() ) {
      clearFixedNodes();
      clearFixedEdges();
      while ( true ) {
        Edgeptr edge = maxCrossingsEdge();
        if ( edge == NULL || allNodesFixed() ) break;
        sprintf( buffer, "->- mce_s, edge %s -> %s",
                 edge->down_node->name, edge->up_node->name );
        tracePrint( edge->up_node->layer, buffer );
        bool last_iteration = false;
        if ( ! isFixedNode( edge->up_node ) ) {
          last_iteration = sift_iteration( edge->up_node );
          fixNode( edge->up_node );
          if ( last_iteration ) return;
        }
        if ( ! isFixedNode( edge->down_node ) ) {
          last_iteration = sift_iteration( edge->down_node );
          fixNode( edge->down_node );
          if ( last_iteration ) return;
        }
        fixEdge( edge );
      }
      tracePrint( -1, "--- mce with sifting, end pass" );
  }
}

/**
 * @return true if the pass should end based on command-line option
 * mce_option; note: if the option is EDGES, maxCrossingsEdge() will return
 * NULL and this will not happen unless all the nodes have been fixed as
 * well, in most cases more than once.
 */
static bool end_mce_pass( Edgeptr edge )
{
  if( edge == NULL ) return true;
  if( mce_option == EARLY 
      && isFixedNode( edge->up_node )
      && isFixedNode( edge->down_node )
      ) return true;
  if( mce_option == NODES
      && allNodesFixed()
      ) return true;
  return false;
}

void maximumCrossingsEdge( void )
{
  tracePrint( -1, "^^^ start maximum crossings edge" );
  while( ! terminate() )
    {
      clearFixedNodes();
      clearFixedEdges();
      while ( true )
        // keep going until specified pass ending is encountered
        {
          // complicated: current implementation visits edges based on layer
          // and node order within layer
          //          if ( randomize_order ) randomize_edge_list();
          Edgeptr edge = maxCrossingsEdge();
          if ( edge == NULL ) break;
          sprintf( buffer, "->- mce, edge %s -> %s",
                   edge->down_node->name, edge->up_node->name );
          tracePrint( edge->up_node->layer, buffer );
          if ( end_mce_pass( edge ) ) break;
          bool last_iteration = false;
          /**
           * @todo what follows is now incorporated into
           * maximumCrossingsEdgeWithSifting(); eventually the two could be
           * merged
           */
          /* if ( sifting_style == TOTAL ) */
          /*   last_iteration = sift_iteration( node ); */
          /* else */
            last_iteration = edge_sift_iteration( edge );
          if ( last_iteration )
            return;
          fixEdge( edge );
        }
      tracePrint( -1, "--- mce, end pass" );
    }
}

void maximumStretchEdge( void ) {
  tracePrint( -1, "^^^ start maximum strech edge with total stretch sifting" );
  while( ! terminate() ) {
      clearFixedNodes();
      clearFixedEdges();
      while ( true ) {
        Edgeptr edge = maxStretchEdge();
        if ( edge == NULL || allNodesFixed() ) break;
        sprintf( buffer, "->- mse, edge %s -> %s",
                 edge->down_node->name, edge->up_node->name );
        tracePrint( edge->up_node->layer, buffer );
        bool last_iteration = false;
        if ( ! isFixedNode( edge->up_node ) ) {
          last_iteration = total_stretch_sift_iteration( edge->up_node );
          fixNode( edge->up_node );
          if ( last_iteration ) return;
        }
        if ( ! isFixedNode( edge->down_node ) ) {
          last_iteration = total_stretch_sift_iteration( edge->down_node );
          fixNode( edge->down_node );
          if ( last_iteration ) return;
        }
        fixEdge( edge );
      }
      tracePrint( -1, "--- mse with sifting, end pass" );
  }
}

// the value used in the Matuszewski et al. paper
#define MAX_FAILS 1

/**
 * Sifts node in decreasing order as determined by the input array
 * @return false if the sift was unsuccessful, i.e., it did not improve upon
 * initial_crossings or if the maximum number of iterations was reached
 *
 * @note here the key is improvement upon the number of crossings at the
 * beginning of this sifting pass, not necessarily the number of crossings
 * overall.
 */
static bool sift_decreasing( const Nodeptr * node_array,
                             int num_nodes, int initial_crossings )
{
#ifdef DEBUG
  printf( "-> sift_decreasing, num_nodes = %d, crossings = %d\n",
          num_nodes, initial_crossings );
#endif
  // sift by decreasing 'weight' (degree in this case)
  int i;
  for( i = num_nodes - 1; i >= 0; i-- )
    {
#ifdef DEBUG
      printf( "  sifting i = %d, node = %s\n", i, node_array[i]->name );
#endif
      sift( node_array[ i ] );
      tracePrint( node_array[ i ]->layer, "^^^ sift_increasing ^^^" );
      sprintf( buffer, " $$$ sift, node = %s, pos = %d",
               node_array[i]->name, node_array[i]->position );
      tracePrint( node_array[ i ]->layer, buffer );
      if ( end_of_iteration() ) break;
    }
#ifdef DEBUG
  printf( "<- sift_decreasing, crossings = %d\n",
          numberOfCrossings() );
#endif
  return numberOfCrossings() < initial_crossings && iteration < max_iterations;
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
static bool sift_increasing( const Nodeptr * node_array,
                             int num_nodes, int initial_crossings )
{
  // sift by increasing 'weight' (degree in this case)
  int i;
  for( i = 0; i < num_nodes; i++ )
    {
      sift( node_array[ i ] );
      tracePrint( node_array[ i ]->layer, "^^^ sift_increasing ^^^" );
      sprintf( buffer, " $$$ sift, node = %s, pos = %d",
               node_array[i]->name, node_array[i]->position );
      tracePrint( node_array[ i ]->layer, buffer );
      if ( end_of_iteration() ) break;
    }
  return numberOfCrossings() < initial_crossings && iteration < max_iterations;
}

void sifting( void ) {
  // sort nodes by increasing degree (other options not implemented yet); but
  // if randomize_order is true, then the order is randomized and the node
  // list is resorted before each pass
  sortByDegree( master_node_list, number_of_nodes );
#ifdef DEBUG
  fprintf(stderr, "  sifting: nodes after sorting -\n" );
  for( index = 0; index < number_of_nodes; index++ )
    fprintf(stderr, "    node_array[%2d] = %s\n", index, node_array[index]->name );
#endif

  /* the sifting algorithm from the Matuszewski et al. paper, except that a
   * fixed number of iterations or a specific runtime limit may supercede the
   * standard stopping criterion
   */
  int fail_count = 0;
  while( ( standard_termination && fail_count < MAX_FAILS )
         || ! terminate() ) {
    int crossings_before = numberOfCrossings();
    bool fail = false;
    if ( randomize_order ) {
      genrand_permute( master_node_list, number_of_nodes, sizeof(Nodeptr) );
      sortByDegree( master_node_list, number_of_nodes );
    }
    fail = ! sift_decreasing( master_node_list, number_of_nodes, crossings_before );
    if ( iteration >= max_iterations )
      break;
    tracePrint( -1, "--- end of sifting pass" );
    if( fail ) {
      fail_count++;
      if ( randomize_order ) {
        genrand_permute( master_node_list, number_of_nodes, sizeof(Nodeptr) );
        sortByDegree( master_node_list, number_of_nodes );
      }
      fail = ! sift_increasing( master_node_list, number_of_nodes,
                                crossings_before );
      if ( end_of_iteration() )
        break;
    }
    else {
      if ( randomize_order ) {
        genrand_permute( master_node_list, number_of_nodes, sizeof(Nodeptr) );
        sortByDegree( master_node_list, number_of_nodes );
      }
      fail = ! sift_decreasing( master_node_list, number_of_nodes,
                                crossings_before );
      if ( end_of_iteration() )
        break;
    }
    tracePrint( -1, "--- end of sifting pass" );
    // wait for mce implementation
/*       if ( randomize_order ) */
/*         RN_permute( node_array, number_of_nodes, sizeof(nodePtr) ); */
    if ( fail ) fail_count++;
  }
}

// preprocessors

void breadthFirstSearch( void )
{
  printf( "bfs not implemented\n" );
}

void depthFirstSearch( void )
{
  assignDfsWeights();
  int layer = 0;
  for( ; layer < number_of_layers; layer++ )
    layerSort( layer );
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

static void swap_nodes( Nodeptr * node_array, int i, int j )
{
  node_array[i]->position = j;
  node_array[j]->position = i;
  Nodeptr temp = node_array[i];
  node_array[i] = node_array[j];
  node_array[j] = temp;
}

/**
 * Does an iteration where either all swaps between nodes i, i+1 on layers L,
 * L+1 are considered where i, L are either even or odd
 * @param crossings the number of crossings before this iteration
 * @param odd_even is 1 if odd values of i and L are considered, even
 * otherwise  
 * @return the number of crossings after this iteration
 */
static int swapping_iteration( int crossings, int odd_even )
{
#ifdef DEBUG
  printf( "-> swapping_iteration, crossings = %d, odd = %d\n",
          crossings, odd_even );
#endif  
  for ( int layer = odd_even; layer < number_of_layers; layer += 2 )
    {
      Layerptr layer_ptr = layers[ layer ];
      for ( int i = odd_even; i <  layer_ptr->number_of_nodes - 1; i += 2 )
        {
          Nodeptr * nodes = layer_ptr->nodes;
          int crossings_before_swap = node_crossings( nodes[i], nodes[i+1] );
          int crossings_after_swap = node_crossings( nodes[i+1], nodes[i] );
          int diff = crossings_before_swap - crossings_after_swap;
          if ( diff > 0 )
            {
              swap_nodes( nodes, i, i+1 );
              crossings -= diff;
            }
#ifdef DEBUG
          printf( " swapping, layer = %d, node = %d, diff = %d, crossings = %d\n",
                  layer, i, diff, crossings );
#endif
        }
      tracePrint( layer, "<-> swapping" );
    }
#ifdef DEBUG
  printf( "<- swapping_iteration, crossings = %d\n",
          crossings );
#endif  
  return crossings;
}


/**
 * !!! attempted to update this so that it would fix the Pareto frontier, but
       a swapping iteration does not actually recompute total crossings !!!
 */
void swapping( void )
{
  bool improved = true;
  post_processing_crossings = numberOfCrossings();
  int previous_best_crossings = post_processing_crossings;
  post_processing_iteration = 0;

#ifdef DEBUG
  printf( "-> swapping, post_processing_crossings = %d\n", post_processing_crossings );
#endif

  tracePrint( -1, "<-> start swapping" );
  while ( improved )
    {
      // look for improvements by swapping nodes i, i+1 on layers L, L+1,
      // where i and L are even
      post_processing_crossings = swapping_iteration( post_processing_crossings, 0 );
      post_processing_iteration++;
      if ( post_processing_crossings < previous_best_crossings )
        {
          improved = true;
          save_order( best_crossings_order );
          previous_best_crossings = post_processing_crossings;
          update_best_all();
        }
      else improved = false;
      post_processing_iteration++;

      // look for improvements by swapping nodes i, i+1 on layers L, L+1,
      // where i and L are odd
      post_processing_crossings = swapping_iteration( post_processing_crossings, 1 );
      if ( post_processing_crossings < previous_best_crossings )
        {
          improved = true;
          save_order( best_crossings_order );
          previous_best_crossings = post_processing_crossings;
          update_best_all();
        }
      // don't set improved to false here -- there may have been improvement
      // during the even iteration
      post_processing_iteration++;
      tracePrint( -1, "-- end of swapping pass" );
    } // while improved

#ifdef DEBUG
  printf( "<- swapping, post_processing_crossings = %d\n", post_processing_crossings );
#endif
  
}

#endif // ! defined(TEST)

/*  [Last modified: 2021 02 15 at 20:50:03 GMT] */
