/**
 * @file stats.c
 * @brief Implementation of functions that print statistics
 * @author Matt Stallmann
 * @date 2009/05/19
 *
 * @todo To keep things simple, total stretch is rounded to an integer value
 */


#include<stdio.h>
#include<stdlib.h>
#include<limits.h>
#include<math.h>
#include<float.h>
#include<string.h>

#include"stats.h"
#include"defs.h"
#include"heuristics.h"
#include"graph.h"
#include"crossings.h"
#include"channel.h"
#include"Statistics.h"
#include"timing.h"
#include"verticality.h"

typedef struct pareto_item {
  double objective_one;
  double objective_two;
  int iteration;
  struct pareto_item * rest;
} * PARETO_LIST;

static PARETO_LIST pareto_list = NULL;

static void init_pareto_list( void ) { pareto_list = NULL; }

/**
 * puts formatted Pareto list to the buffer, starting with Pareto tag
 */
static void putParetoList(char * buffer) {
    PARETO_LIST local_list = pareto_list;
    char local_buffer[MAX_NAME_LENGTH];
    *buffer = '\0';
    strcat(buffer, "Pareto,");
    while ( local_list != NULL ) {
        if ( pareto_objective == BOTTLENECK_TOTAL ) {
            sprintf(local_buffer, "%d^%d",
                    (int) local_list->objective_one,
                    (int) local_list->objective_two);
        }
        else if ( pareto_objective == TOTAL_STRETCH ) {
            sprintf(local_buffer, "%d^%f",
                    (int) local_list->objective_one,
                    local_list->objective_two);
        }
        else if ( pareto_objective == BOTTLENECK_STRETCH ) {
            sprintf(local_buffer, "%d^%f",
                    (int) local_list->objective_one,
                    local_list->objective_two);
        }
        else if ( pareto_objective == TOTAL_VERTICAL ) {
            sprintf(local_buffer, "%d^%ld",
                    (int) local_list->objective_one,
                    (long) local_list->objective_two);
        }
        else if ( pareto_objective == BOTTLENECK_VERTICAL ) {
            sprintf(local_buffer, "%d^%ld",
                    (int) local_list->objective_one,
                    (long) local_list->objective_two);
        }
        strcat(buffer, local_buffer);
        local_list = local_list->rest;
        if ( local_list != NULL ) strcat(buffer, ";");
    }
    local_list = pareto_list;
    strcat(buffer, ", ");
    while ( local_list != NULL ) {
        sprintf(local_buffer, "%d", local_list->iteration);
        strcat(buffer, local_buffer);
        local_list = local_list->rest;
        if ( local_list != NULL ) strcat(buffer, ";");
    }
}

/**
 * Inserts, if appropriate, an item with the given values of objective_one and
 * and objective_two into the list representing a Pareto frontier. The frontier
 * is maintained in increasing objective_one, decreasing objective_two order.
 */
static PARETO_LIST pareto_insert(double objective_one,
                                 double objective_two,
                                 int iteration,
                                 PARETO_LIST list) {
#ifdef DEBUG
  printf("-> pareto_insert: %f, %f, %d, ",
         objective_one, objective_two, iteration);
  //  print_pareto_list(list, stdout);
  printf("\n");
#endif
  PARETO_LIST new_list = NULL;
  if ( list == NULL ) {
    new_list = (PARETO_LIST) calloc(1, sizeof(struct pareto_item));
    new_list->objective_one = objective_one;
    new_list->objective_two = objective_two;
    new_list->iteration = iteration;
    new_list->rest = NULL;
  }
  else {
    double first_objective_one = list->objective_one;
    double first_objective_two = list->objective_two;
    if ( objective_one < first_objective_one
         && objective_two > first_objective_two ) {
      // new pareto point
      new_list = (PARETO_LIST) calloc(1, sizeof(struct pareto_item));
      new_list->objective_one = objective_one;
      new_list->objective_two = objective_two;
      new_list->iteration = iteration;
      new_list->rest = list;
    }
    else if ( objective_one < first_objective_one
              && objective_two == first_objective_two ) {
      // replace first point, found one with smaller objective_one value
      list->objective_one = objective_one;
      list->iteration = iteration;
      new_list = list;
    }
    else if ( objective_one <= first_objective_one
              && objective_two < first_objective_two ) {
      // replace first point with better one; since the new point also has
      // smaller objective_two, it may replace others down the line; in this
      // case, we need to actually delete the existing first point
      new_list = pareto_insert(objective_one,
                               objective_two,
                               iteration,
                               list->rest);
      free(list);
    }
    else if ( objective_one > first_objective_one
              && objective_two < first_objective_two ) {
      // need to keep looking; point with greater or equal objective_one not
      // found
      list->rest = pareto_insert(objective_one,
                                 objective_two,
                                 iteration,                                 
                                 list->rest);
      new_list = list;
    }
    else {
      // otherwise, no need to continue: objective_one >= first_objective_one and
      // objective_two >= first_objective_two
      new_list = list;
    }
  }
#ifdef DEBUG
  printf("<- pareto_insert: ");
  //  print_pareto_list(new_list, stdout);
  printf("\n");
#endif
  return new_list;
}

/**
 * recursive helper function for deallocation of the linked list
 */
static void deallocatePLhelper(PARETO_LIST list) {
    if ( list == NULL ) return;
    deallocatePLhelper(list->rest);
    free(list);
}

void deallocateParetoList(void) {
    deallocatePLhelper(pareto_list);
}

CROSSING_STATS_INT total_crossings;
CROSSING_STATS_INT bottleneck_crossings;
CROSSING_STATS_INT favored_edge_crossings;
CROSSING_STATS_LONG total_nonverticality;
CROSSING_STATS_INT bottleneck_verticality;
CROSSING_STATS_DOUBLE total_stretch;
CROSSING_STATS_DOUBLE bottleneck_stretch;
Statistics overall_degree;

static void init_specific_crossing_stats_int( CROSSING_STATS_INT * stats,
                                              const char * name )
{
  stats->name = name;
  stats->at_beginning = INT_MAX;
  stats->after_preprocessing = INT_MAX;
  stats->after_heuristic = INT_MAX;
  stats->after_post_processing = INT_MAX;
  stats->best = INT_MAX;
  stats->previous_best = INT_MAX;
  stats->best_heuristic_iteration = -1;
  stats->post_processing_iteration = -1;
}
static void init_specific_crossing_stats_long( CROSSING_STATS_LONG * stats,
                                              const char * name )
{
  stats->name = name;
  stats->at_beginning = LONG_MAX;
  stats->after_preprocessing = LONG_MAX;
  stats->after_heuristic = LONG_MAX;
  stats->after_post_processing = LONG_MAX;
  stats->best = LONG_MAX;
  stats->previous_best = LONG_MAX;
  stats->best_heuristic_iteration = -1;
  stats->post_processing_iteration = -1;
}

static void init_specific_crossing_stats_double( CROSSING_STATS_DOUBLE * stats,
                                                 const char * name )
{
  stats->name = name;
  stats->at_beginning = DBL_MAX;
  stats->after_preprocessing = DBL_MAX;
  stats->after_heuristic = DBL_MAX;
  stats->after_post_processing = DBL_MAX;
  stats->best = DBL_MAX;
  stats->previous_best = DBL_MAX;
  stats->best_heuristic_iteration = -1;
  stats->post_processing_iteration = -1;
}

void init_crossing_stats( void )
{
  init_specific_crossing_stats_int( & total_crossings, "Crossings" );
  init_specific_crossing_stats_int( & bottleneck_crossings, "BottleneckCrossings" );
  init_specific_crossing_stats_long( & total_nonverticality, "NonVerticality" );
  init_specific_crossing_stats_int( & bottleneck_verticality, "BottleneckVerticality" );
  init_specific_crossing_stats_double( & total_stretch, "Stretch" );
  init_specific_crossing_stats_double( & bottleneck_stretch, "BottleneckStretch" );
  if ( pareto_objective != NO_PARETO )
    init_pareto_list();
}

void capture_beginning_stats( void )
{
  total_crossings.at_beginning = numberOfCrossings();
  bottleneck_crossings.at_beginning = maxEdgeCrossings();
  total_nonverticality.at_beginning = updateAllVerticality();
  bottleneck_verticality.at_beginning = getBottleneckNonverticality();
  total_stretch.at_beginning = totalStretch();
  bottleneck_stretch.at_beginning = maxEdgeStretch();
}

void capture_preprocessing_stats( void )
{
  total_crossings.after_preprocessing = numberOfCrossings();
  bottleneck_crossings.after_preprocessing = maxEdgeCrossings();
  total_nonverticality.after_preprocessing = updateAllVerticality();
  bottleneck_verticality.after_preprocessing = getBottleneckNonverticality();
  total_stretch.after_preprocessing = totalStretch();
  bottleneck_stretch.after_preprocessing = maxEdgeStretch();
}

void capture_heuristic_stats( void )
{
  total_crossings.after_heuristic = total_crossings.best;
  bottleneck_crossings.after_heuristic = bottleneck_crossings.best;
  total_nonverticality.after_heuristic = total_nonverticality.best;
  bottleneck_verticality.after_heuristic = bottleneck_verticality.best;
  total_stretch.after_heuristic = total_stretch.best;
  bottleneck_stretch.after_heuristic = bottleneck_stretch.best;
}

void capture_post_processing_stats( void )
{
  // the post processing iterations are merely counted since the total number
  // of crossings improves after each one by definition; not so with
  // bottleneck crossings or stretch
  total_crossings.after_post_processing = total_crossings.best;
  bottleneck_crossings.after_post_processing = bottleneck_crossings.best;
  total_nonverticality.after_post_processing = total_nonverticality.best;
  bottleneck_verticality.after_post_processing = bottleneck_verticality.best;
  total_stretch.after_post_processing = total_stretch.best;
  bottleneck_stretch.after_post_processing = bottleneck_stretch.best;
}

void update_best_int( CROSSING_STATS_INT * stats, Positionptr pos_info,
                      int (* crossing_retrieval_function) (void) )
{
#ifdef DEBUG
  printf("-> update_best_int, %s, %d\n", stats->name, stats->best);
#endif
  int current_value = crossing_retrieval_function();
  fprintf(stderr, "  $ update best int, current_value = %d\n", current_value);
  if ( stats->best == 34 ) abort();
  if( current_value < stats->best )
    {
      stats->best = current_value;
      if ( post_processing_iteration < 0 )
        stats->best_heuristic_iteration = iteration;
      else
        stats->post_processing_iteration = post_processing_iteration;
      savePositions( pos_info );
    }
#ifdef DEBUG
  printf("<- update_best_int, %s, %d iter = %d pp_iter = %d\n", stats->name, stats->best,
  stats->best_heuristic_iteration, stats->post_processing_iteration);
#endif  
}

void update_best_long( CROSSING_STATS_LONG * stats, Positionptr pos_info,
                      long (* crossing_retrieval_function) (void) )
{
#ifdef DEBUG
  printf("-> update_best_long, %s, %ld\n", stats->name, stats->best);
#endif
  long current_value = crossing_retrieval_function();
  if( current_value < stats->best )
    {
      stats->best = current_value;
      if ( post_processing_iteration < 0 )
        stats->best_heuristic_iteration = iteration;
      else
        stats->post_processing_iteration = post_processing_iteration;
      savePositions( pos_info );
    }
#ifdef DEBUG
  printf("<- update_best_long, %s, %ld iter = %d pp_iter = %d\n", stats->name, stats->best,
  stats->best_heuristic_iteration, stats->post_processing_iteration);
#endif  
}

void update_best_double( CROSSING_STATS_DOUBLE * stats, Positionptr pos_info,
                         double (* crossing_retrieval_function) (void) )
{
#ifdef DEBUG
  printf("-> update_best_double, %s, %f\n", stats->name, stats->best);
#endif
  double current_value = crossing_retrieval_function();
  if( current_value < stats->best )
    {
      stats->best = current_value;
      if ( post_processing_iteration < 0 )
        stats->best_heuristic_iteration = iteration;
      else
        stats->post_processing_iteration = post_processing_iteration;
      savePositions( pos_info );
    }
#ifdef DEBUG
  printf("<- update_best_double, %s, %f\n", stats->name, stats->best);
#endif  
}

/**
 * @todo There are some obvious inefficiencies here:
 * - totalStretch() and maxEdgeStretch() independently compute stretch
 * for each edge; it makes no sense to call both, never mind calling
 * both twice in case of Pareto updates
 * - it makes no sense to compute and report stretch information if
 * not relevant; not doing so cuts runtime in half
 * - the same holds for maxEdgeCrossings()
 * - total crossings are maintained and updated in a variety of
 * places, all of which eventually call change_crossings(), where an
 * insertion sort is done to count inversions
 * - maxEdgeCrossings() relies on the fact that total crossings have
 * been properly maintained; change_crossings() is used to update
 * crossings for a single node or edge when an algorithm does a swap
 */
void update_best_all(void) {
#ifdef DEBUG
  fprintf(stderr, "-> update_best_all, crossings = %d\n", numberOfCrossings());
#endif
  update_best_int( & total_crossings, best_crossings_order, numberOfCrossings );
  update_best_int( & bottleneck_crossings,
                   best_edge_crossings_order, maxEdgeCrossings );
  update_best_long( & total_nonverticality, best_nonverticality_order,
		  	  	   updateAllVerticality );
  update_best_int(& bottleneck_verticality, best_bottleneck_verticality_order, getBottleneckNonverticality);
  update_best_double( & total_stretch, best_total_stretch_order, totalStretch );
  update_best_double( & bottleneck_stretch,
                      best_bottleneck_stretch_order, maxEdgeStretch );
  if ( pareto_objective == BOTTLENECK_TOTAL ) {
    pareto_list = pareto_insert( maxEdgeCrossings(),
                                 numberOfCrossings(),
                                 iteration,
                                 pareto_list );
  }
  else if ( pareto_objective == TOTAL_STRETCH && post_processing_iteration < 1 ) {
    // don't do this if in post processing - leads to way too many Pareto points
    // and causes stack overflow for larger instances
    pareto_list = pareto_insert( numberOfCrossings(),
                                 totalStretch(),
                                 iteration,
                                 pareto_list );
  }
  else if ( pareto_objective == BOTTLENECK_STRETCH ) {
    pareto_list = pareto_insert( maxEdgeCrossings(),
                                 totalStretch(),
                                 iteration,
                                 pareto_list );
  }
  else if ( pareto_objective == TOTAL_VERTICAL ) {
    pareto_list = pareto_insert( numberOfCrossings(),
                                 updateAllVerticality(),
                                 iteration,
                                 pareto_list );
  }
  else if ( pareto_objective == BOTTLENECK_VERTICAL ) {
    pareto_list = pareto_insert( maxEdgeCrossings(),
                                 updateAllVerticality(),
                                 iteration,
                                 pareto_list );
  }
#ifdef DEBUG
  fprintf(stderr, "<- update_best_all, crossings = %d\n", numberOfCrossings());
#endif
}

bool has_improved_long( CROSSING_STATS_LONG * stats )
{
#ifdef DEBUG
  printf( "-> has_improved_long, stats = %s, best = %ld, previous = %ld\n",
          stats->name, stats->best, stats->previous_best );
#endif
  bool improved = false;
  if ( stats->best < stats->previous_best ) {
    improved = true;
    stats->previous_best = stats->best;
  }
#ifdef DEBUG
  printf( "<- has_improved, return %d, best = %ld, previous = %ld, iteration = %d\n",
          improved, stats->best, stats->previous_best, iteration );
#endif
  return improved;
}

bool has_improved_int( CROSSING_STATS_INT * stats )
{
#ifdef DEBUG
  printf( "-> has_improved_int, stats = %s, best = %d, previous = %d\n",
          stats->name, stats->best, stats->previous_best );
#endif
  bool improved = false;
  if ( stats->best < stats->previous_best ) {
    improved = true;
    stats->previous_best = stats->best;
  }
#ifdef DEBUG
  printf( "<- has_improved, return %d, best = %d, previous = %d, iteration = %d\n",
          improved, stats->best, stats->previous_best, iteration );
#endif
  return improved;
}

bool has_improved_double( CROSSING_STATS_DOUBLE * stats )
{
#ifdef DEBUG
  printf( "-> has_improved_double, stats = %s, best = %f, previous = %f\n",
          stats->name, stats->best, stats->previous_best );
#endif
  bool improved = false;
  if ( stats->best < stats->previous_best ) {
    improved = true;
    stats->previous_best = stats->best;
  }
#ifdef DEBUG
  printf( "<- has_improved_double, return %d, best = %f, previous = %f, iteration = %d\n",
          improved, stats->best, stats->previous_best, iteration );
#endif
  return improved;
}

static int total_layer_degree( int layer )
{
  int total_degree = 0;
  int position = 0;
  for( ; position < layers[ layer ]->number_of_nodes; position++ )
    {
      Nodeptr node = layers[ layer ]->nodes[ position ];
      total_degree += DEGREE( node );
    }
  return total_degree;
}

static void add_layer_degrees( int layer, Statistics s )
{
  int position = 0;
  for( ; position < layers[ layer ]->number_of_nodes; position++ )
    {
      Nodeptr node = layers[ layer ]->nodes[ position ];
      if ( DEGREE( node ) > 0 )
        add_data( s, DEGREE( node ) );
    }
}

static void print_layer_degree_statistics( int layer, FILE * output_stream )
{
  int layer_size = layers[layer]->number_of_nodes;
  Nodeptr * nodes = layers[ layer ]->nodes;
  Statistics layer_degree = init_statistics( layer_size );
  int position = 0;
  for( ; position < layer_size; position++ )
    {
      Nodeptr node = nodes[ position ];
      if ( DEGREE( node ) > 0 )
        add_data( layer_degree, DEGREE( node ) );
    }
  fprintf(output_stream, "Stat,NDegree,%s,%d,", graph_name, layer);
  print_statistics(layer_degree, output_stream, "%0.2lf");
  fprintf(output_stream, "\n");
  deallocateStatistics(layer_degree);
}

static void print_channel_degree_statistics( FILE * output_stream )
{
  Statistics channel_degree_discrepancy
    = init_statistics( number_of_layers - 1 );
  for ( int layer = 1; layer < number_of_layers; layer++ )
    {
      Layerptr upper_layer = layers[layer];
      Layerptr lower_layer = layers[layer - 1];
      int upper_layer_size = upper_layer->number_of_nodes;
      int lower_layer_size = lower_layer->number_of_nodes;
      Statistics channel_degree
        = init_statistics( upper_layer_size + lower_layer_size );
      for ( int i = 0; i < upper_layer_size; i++ )
        {
          Nodeptr node = upper_layer->nodes[i];
          if ( node->down_degree > 0 )
            add_data( channel_degree, node->down_degree );
        }
      for ( int i = 0; i < lower_layer_size; i++ )
        {
          Nodeptr node = lower_layer->nodes[i];
          if ( node->up_degree > 0 )
            add_data( channel_degree, node->up_degree );
        }
      fprintf(output_stream, "Stat,CDegree,%s,%d,", graph_name, layer);
      print_statistics(channel_degree, output_stream, "%0.2lf");
      fprintf(output_stream, "\n");
      add_data( channel_degree_discrepancy,
                get_max( channel_degree ) / get_median( channel_degree ) );
      deallocateStatistics( channel_degree );
    }
  fprintf(output_stream, "Stat,AvgCDegreeDisc,%s,%d,", graph_name,-1);
  print_statistics(channel_degree_discrepancy, output_stream, "%0.2f");
      fprintf(output_stream, "\n");
  deallocateStatistics( channel_degree_discrepancy );
}

static void print_channel_edge_counts( FILE * output_stream ) {
  Statistics edge_count = init_statistics(number_of_layers - 1);
  for ( int layer = 1; layer < number_of_layers; layer++ ) {
    Layerptr upper_layer = layers[layer];
    int upper_layer_size = upper_layer->number_of_nodes;
    // count the number of edges into the channel from the upper layer (these
    // are the same as the edges from the lower layer into the channel)
    int edges_from_upper_layer = 0;
    for ( int i = 0; i < upper_layer_size; i++ ) {
      Nodeptr node = upper_layer->nodes[i];
      edges_from_upper_layer += node->down_degree;
    }
    fprintf(output_stream, "Stat,EdgesInChannel,%s,%d,%d\n",
            graph_name, layer,
            edges_from_upper_layer);
    add_data(edge_count, edges_from_upper_layer);
  }
  fprintf(output_stream, "Stat,SEdgesInChannel,%s,%d,", graph_name,-1);
  print_statistics(edge_count, output_stream, "%0.2f");
      fprintf(output_stream, "\n");
  deallocateStatistics(edge_count);
}

static void compute_degree_statistics( void )
{
  for( int layer = 0; layer < number_of_layers; layer++ )
    {
      for( int position = 0; position < layers[ layer ]->number_of_nodes; position++ )
        {
          Nodeptr node = layers[ layer ]->nodes[ position ];
          add_data( overall_degree, DEGREE( node ) );
        }
    }
}

static void print_degree_statistics( FILE * output_stream )
{
  Statistics nodes_per_layer = init_statistics( number_of_layers );
  Statistics layer_degrees = init_statistics( number_of_layers );
  for ( int i = 0; i < number_of_layers; i++ )
    {
      add_layer_degrees( i, overall_degree );
      add_data( nodes_per_layer, layers[i]->number_of_nodes );
      add_data( layer_degrees, total_layer_degree( i ) );
      print_layer_degree_statistics( i, output_stream );
    }
  fprintf(output_stream, "Stat,LDegree,%s,%d,", graph_name, -1);
  print_statistics(layer_degrees, output_stream, "%0.2lf");
  fprintf(output_stream, "\n");
  fprintf(output_stream, "Stat,TDegree,%s,%d,", graph_name, -1);
  print_statistics(overall_degree, output_stream, "%0.2lf");
  fprintf(output_stream, "\n");
  fprintf(output_stream, "Stat,PerLayerNodes,%s,%d,", graph_name, -1);
  print_statistics(nodes_per_layer, output_stream, "%0.2lf");
  fprintf(output_stream, "\n");
  deallocateStatistics(layer_degrees);
  deallocateStatistics(nodes_per_layer);
}

// The following has not been used or tested

/**
 * @todo minimize maximum crossings on any layer
 */
/* static void print_layer_crossing_statistics( int layer, FILE * output_stream ) */
/* { */
/* }  */

void print_comments(FILE * output_stream) {
    fprintf(output_stream, "# comments\n");
    fprintf(output_stream, "%s", comments);
    fprintf(output_stream, "# end_comments\n");
}

void print_graph_statistics( FILE * output_stream )
{
    if ( strlen(comments) > 0 ) {
        print_comments(output_stream);
    }
  int effective_number_of_nodes = number_of_nodes - number_of_isolated_nodes;
  fprintf( output_stream, "GraphName,%s\n", graph_name );
  fprintf( output_stream, "NumberOfLayers,%d\n", number_of_layers );
  fprintf( output_stream, "NumberOfNodes,%d\n", number_of_nodes );
  fprintf( output_stream, "IsolatedNodes,%d\n", number_of_isolated_nodes );
  fprintf( output_stream, "EffectiveNodes,%d\n", effective_number_of_nodes );
  fprintf( output_stream, "NumberOfEdges,%d\n", number_of_edges );
  fprintf( output_stream, "EdgeDensity,%2.2f\n",
          (double) number_of_edges / effective_number_of_nodes );
  overall_degree = init_statistics( number_of_nodes );
  if ( verbose ) {
      fprintf(output_stream, "Tag,Type,graph_name,layer,min,median,mean,max,stdev,N\n"
      "   Tag = Stat to make it easy to grep\n"
      "   NDegree is for nodes on a layer, CDegree for edges in a channel, defined by its upper layer\n"
      "   LDegree is total degree per layer, TDegree is total degree per node\n"
      "   perLayerNodes is self-explanatory\n"
      "   AvgCDegreeDisc is average over channels of *degree discrepancy*\n"
      "     defined as max_degree / median_degree\n");
      print_degree_statistics( output_stream );
      print_channel_degree_statistics( output_stream );
      print_channel_edge_counts( output_stream );
  }
  else
    compute_degree_statistics();
  fprintf( output_stream, "MinDegree,%d\n", (int) get_min( overall_degree ) );
  fprintf( output_stream, "MaxDegree,%d\n", (int) get_max( overall_degree ) );
  fprintf( output_stream, "MeanDegree,%2.2f\n", get_mean( overall_degree ) );
  fprintf( output_stream, "MedianDegree,%2.1f\n", get_median( overall_degree ) );
  deallocateStatistics( overall_degree );
}

static void print_crossing_stats_int(FILE * output_stream,
                                     CROSSING_STATS_INT stats) {
  fprintf( output_stream, "Start%s,%d\n", stats.name, stats.at_beginning );
  fprintf( output_stream, "Pre%s,%d\n", stats.name, stats.after_preprocessing );
  fprintf( output_stream, "Heuristic%s,%d,iteration,%d\n",
           stats.name, stats.after_heuristic, stats.best_heuristic_iteration );
  fprintf( output_stream, "Post%s,%d,iteration,%d\n",
           stats.name, stats.after_post_processing, stats.post_processing_iteration );
}

static void print_crossing_stats_long(FILE * output_stream,
                                     CROSSING_STATS_LONG stats) {
  fprintf( output_stream, "Start%s,%ld\n", stats.name, stats.at_beginning );
  fprintf( output_stream, "Pre%s,%ld\n", stats.name, stats.after_preprocessing );
  fprintf( output_stream, "Heuristic%s,%ld,iteration,%d\n",
           stats.name, stats.after_heuristic, stats.best_heuristic_iteration );
  fprintf( output_stream, "Post%s,%ld,iteration,%d\n",
           stats.name, stats.after_post_processing, stats.post_processing_iteration );
}

static void print_crossing_stats_double(FILE * output_stream,
                                        CROSSING_STATS_DOUBLE stats) {
  fprintf( output_stream, "Start%s,%f\n", stats.name, stats.at_beginning );
  fprintf( output_stream, "Pre%s,%f\n", stats.name, stats.after_preprocessing );
  fprintf( output_stream, "Heuristic%s,%f,iteration,%d\n",
           stats.name, stats.after_heuristic, stats.best_heuristic_iteration );
  fprintf( output_stream, "Post%s,%f,iteration,%d\n",
           stats.name, stats.after_post_processing, stats.post_processing_iteration );
}

void getParetoList(char * buffer) {
    *buffer = '\0';
    strcat(buffer, "Pareto,");
    putParetoList(buffer);
}

void print_run_statistics( FILE * output_stream )
{
    fprintf( output_stream, "Preprocessor,%s\n", preprocessor );
    fprintf( output_stream, "Heuristic,%s\n", heuristic );
    fprintf( output_stream, "Iterations,%d\n", iteration );
    fprintf( output_stream, "Runtime,%2.3f\n", RUNTIME );
    
    print_crossing_stats_int( output_stream, total_crossings );
    print_crossing_stats_int( output_stream, bottleneck_crossings );
    print_crossing_stats_long( output_stream, total_nonverticality );
    print_crossing_stats_int( output_stream, bottleneck_verticality );
    print_crossing_stats_double( output_stream, total_stretch );
    print_crossing_stats_double( output_stream, bottleneck_stretch );

    if ( pareto_objective != NO_PARETO ) {
        char buffer[MAX_NAME_LENGTH];
        getParetoList(buffer);
        fprintf(output_stream, "%s\n", buffer);
    }
}
