/**
 * @file main.c
 * @brief Main program for heuristics minimizing various objectives in
 * layered graphs
 * @author Matt Stallmann
 * @date 2008/12/29
 */


#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>              /* getopt() */
#include<getopt.h>              /* for Linux */
#include<limits.h>
#include<assert.h>
#include<libgen.h>              /* basename() */
#include<float.h>               /* DBL_MAX */

#include"constants.h"
#include"stats.h"
#include"defs.h"
#include"heuristics.h"
#include"graph_io.h"
#include"graph.h"
#include"crossings.h"
#include"channel.h"
#include"order.h"
#include"priority_edges.h"
#include"timing.h"
#include"random.h"

// definition of command-line options with default values

char * heuristic = "";
char * preprocessor = "";
int max_iterations = INT_MAX;
double runtime = 0;
double max_runtime = DBL_MAX;
double start_time = 0;
bool standard_termination = true;
enum adjust_weights_enum adjust_weights = LEFT;
enum sift_option_enum sift_option = DEGREE;
enum mce_option_enum mce_option = NODES;
enum sifting_style_enum sifting_style = DEFAULT;
enum pareto_objective_enum pareto_objective = NO_PARETO;
int capture_iteration = INT_MIN; /* because -1 is a possible iteration */
bool favored_edges = false;
bool randomize_order = false;
/**
 * @todo not clear which value of this option works best; stay tuned ...
 */
bool balanced_weight = false;
bool produce_output = false;
char * output_base_name = NULL;

bool verbose = false;
int trace_freq = -1;

// definition of order saving structures
Orderptr best_crossings_order = NULL;
Orderptr best_edge_crossings_order = NULL;
Orderptr best_total_stretch_order = NULL;
Orderptr best_bottleneck_stretch_order = NULL;
Orderptr best_favored_crossings_order = NULL;

/** buffer to be used for all output file names */
static char output_file_name[MAX_NAME_LENGTH];

static bool do_post_processing = false;

/**
 * prints usage message
 *
 * @todo sort these alphabetically and make them more meaningful and/or use
 * long options
 */
static void printUsage( void )
{
  printf( "Usage: minimization [opts] file.dot file.ord\n" );
  printf( " where opts is one or more of the following\n" );
  printf(
         "  -h (median | bary | mod_bary | mcn | sifting | mce | mce_s | mse\n"
         "     | static_bary | alt_bary | up_down_bary | rotate_bary | slab_bary {parallel barycenter versions})\n"
         "     [main heuristic - default none]\n"
         "  -p (bfs | dfs | mds) [preprocessing - default none]\n"
         "  -z if post processing (repeated swaps until no improvement) is desired\n"
         "  -i MAX_ITERATIONS [stop if no improvement]\n"
         "  -R SEED edge list, node list, or sequence of layers will be randomized\n"
         "     after each pass of mod_bary, mce, mcn, mse, sifting, etc.\n"
         "     to break ties differently when sorting; SEED is an integer seed\n"
         "  -r SECONDS = maximum runtime [stop if no improvement]\n"
         "  -c ITERATION [capture the order after this iteration in a file]\n"
         "  -P PARETO_OBJECTIVES (b_t | s_t | b_s) pair of objectives for Pareto optima\n"
         "      b = bottleneck, t = total, s = stretch (default = none)\n"
         "  -o BASE produce file(s) with name(s) BASE-h.ord, where h is the heuristic used\n"
         "     -o _ (underscore) means use the base name of the dot file\n"
         "  -w (none | avg | left) [adjust weights in barycenter, default left, but avg in parallel versions]\n"
         "  -b average the averages of the two neighboring layers when computing barycenter weights\n"
         "     [this is the default for parallel versions]\n"
         "  -s (layer | degree | random) [sifting variation - see paper]\n"
         "  -e (nodes | edges | early | one_node )\n"
         "     [mce variation - default is nodes: pass ends when all nodes are marked]\n"
         "  -g (total | max) [what sifting is based on] [default: total for sifting, mcn; max for mce]\n"
         "      [not implemented yet]\n"
         "  -v to get verbose information about the graph\n"
         "  -t trace_freq, if trace printout is desired, 0 means only at the end of a pass, > 0 sets frequency\n"
         );
}

/**
 * Truncate the string in buffer so that it ends before the first '.' and
 * copy it into the buffer.
 *
 * @return a pointer to the position of the last '/' so as to remove the
 * directory component of the name in the buffer
 *
 * Used to emulate the 'basename' command
 *
 * @todo there must be a better way to do this, but basename() by itself does
 * not work.
 */
static char * base_name( char * buffer )
{
  char * without_directory = basename( buffer );
  char * pointer_to_last_period = strrchr( without_directory, '.' );
  *pointer_to_last_period = '\0';
  return without_directory;
}

static void runPreprocessor( void )
{
  printf( "--- Running preprocessor %s\n", preprocessor );
  if( strcmp( preprocessor, "" ) == 0 )
    ;                           /* do nothing */
  else if( strcmp( preprocessor, "bfs" ) == 0 )
    breadthFirstSearch();
  else if( strcmp( preprocessor, "dfs" ) == 0 )
    depthFirstSearch();
  else if( strcmp( preprocessor, "mds" ) == 0 )
    middleDegreeSort();
  else
    {
      printf( "Bad preprocessor '%s'\n", preprocessor );
      printUsage();
      exit( EXIT_FAILURE );
    }
}

/**
 * @todo It would be nice to have a way to run two heuristics, one after the
 * other. Not really needed - can always use the output file of one as input
 * to the other
 */
static void runHeuristic( void )
{
  printf( "=== Running heuristic %s\n", heuristic );
  if( strcmp( heuristic, "" ) == 0 )
    ;                           /* do nothing */
  else if( strcmp( heuristic, "median" ) == 0 )
    median();
  else if( strcmp( heuristic, "bary" ) == 0 )
    barycenter();
  else if( strcmp( heuristic, "mod_bary" ) == 0 )
    modifiedBarycenter();
  else if ( strcmp( heuristic, "mcn" ) == 0 )
    maximumCrossingsNode();
  else if ( strcmp( heuristic, "mce_s" ) == 0 )
    maximumCrossingsEdgeWithSifting();
  else if ( strcmp( heuristic, "sifting" ) == 0 )
    sifting();
  else if( strcmp( heuristic, "mce" ) == 0 ) {
    maximumCrossingsEdge();
  }
  else if( strcmp( heuristic, "mse" ) == 0 ) {
    maximumStretchEdge();
  }
  else {
      printf( "Bad heuristic '%s'\n", heuristic );
      printUsage();
      exit( EXIT_FAILURE );
  }
}

/**
 * As of now, the main program does the following seqence of events -
 * -# Read the DOT file
 * -# Read the ORD file
 * -# Display the attributes of the graph
 * -# Count the number of crossings.
 * -# Apply a preprocess and a heuristic (both optional) on the graph
 * -# Optionally apply a post-processor that repeatedly swaps neighboring
 * nodes until there's no more improvement
 * -# Count the number of crossings after each phase (and save the ORD files
 * for the minimum number of crossings in each phase)
 */
int main( int argc, char * argv[] )
{
  printf("################################################################\n");
  printf("########### min_crossings, release 1.1, 2016/05/23 #############\n");

  int seed = 0;
  int ch = -1;

  // process command-line options; these must come before the file arguments
  // note: options that have an arg are followed by : but others are not
  while ( (ch = getopt(argc, argv, "bc:e:fgh:i:k:o:p:P:R:r:s:t:vw:zm:")) != -1)
    {
      switch(ch)
        {
        case 'h':
          heuristic = optarg;
          break;

        case 'p':
          preprocessor = optarg;
          break;

        case 'z':
          do_post_processing = true;
          break; 

        case 'i':
          max_iterations = atoi( optarg );
          standard_termination = false;
          break;

        case 'R':
          seed = atoi( optarg );
          init_genrand( seed );
          randomize_order = true;
          break;

        case 'r':
          max_runtime = atof( optarg );
          standard_termination = false;
          break;

        case 'P':
          if ( strcmp( optarg, "b_t" ) == 0 ) pareto_objective = BOTTLENECK_TOTAL;
          else if ( strcmp( optarg, "s_t" ) == 0 ) pareto_objective = STRETCH_TOTAL; 
          else if ( strcmp( optarg, "b_s" ) == 0 ) pareto_objective = BOTTLENECK_STRETCH; 
          else {
            printf( "Bad value '%s' for option -P\n", optarg );
            printUsage();
            exit( EXIT_FAILURE );
          }
          break;

        case 'c':
          capture_iteration = atoi( optarg );
          break;

        case 'w':
          if( strcmp( optarg, "none" ) == 0 ) adjust_weights = NONE;
          else if( strcmp( optarg, "avg" ) == 0 ) adjust_weights = AVG; 
          else if( strcmp( optarg, "left" ) == 0 ) adjust_weights = LEFT;
          else
            {
              printf( "Bad value '%s' for option -w\n", optarg );
              printUsage();
              exit( EXIT_FAILURE );
            }
          break;
        case 'b':
          balanced_weight = true;
          break;
        case 's':
          if( strcmp( optarg, "layer" ) == 0 ) sift_option = LAYER;
          else if( strcmp( optarg, "degree" ) == 0 ) sift_option = DEGREE; 
          else if( strcmp( optarg, "random" ) == 0 ) sift_option = RANDOM;
          else
            {
              printf( "Bad value '%s' for option -s\n", optarg );
              printUsage();
              exit( EXIT_FAILURE );
            }
          break;
        case 'e':
          if( strcmp( optarg, "nodes" ) == 0 ) mce_option = NODES;
          else if( strcmp( optarg, "edges" ) == 0 ) mce_option = EDGES; 
          else if( strcmp( optarg, "early" ) == 0 ) mce_option = EARLY;
          else if( strcmp( optarg, "one_node" ) == 0 ) mce_option = ONE_NODE;
          else
            {
              printf( "Bad value '%s' for option -e\n", optarg );
              printUsage();
              exit( EXIT_FAILURE );
            }
          break;
        case 'g':
          if( strcmp( optarg, "total" ) == 0 ) sifting_style = TOTAL;
          else if( strcmp( optarg, "max" ) == 0 ) sifting_style = MAX; 
          else {
            printf( "Bad value '%s' for option -g\n", optarg );
            printUsage();
            exit( EXIT_FAILURE );
          }
          break;
        case 'o':
          produce_output = true;
          output_base_name = calloc( strlen(optarg) + 1, sizeof(char) );
          strcpy( output_base_name, optarg );
          break;
        case 'v':
          verbose = true;
          break;
        case 't':
          trace_freq = atoi( optarg );
          break;
        default:
          printUsage();
          exit( EXIT_FAILURE );
          break;

        }  /* end of switch */
    }  /* end of while */

  // start command line at first index after the options and get the two file
  // names: dot and ord, respectively
  argc -= optind;
  argv += optind;

  /**
   * @todo Allow either one or two arguments; extract base name and
   * extension; if only one file and extension is .sgf, read sgf; otherwise
   * read dot and ord.
   */
  if ( argc == 2 ) {
      const char * dot_file_name = argv[0];
      const char * ord_file_name = argv[1];

      // handle special case where user specified an empty (_) base name for output
      /**
       * @todo need to handle the same situation for sgf, but that can wait
       */
      if ( produce_output
           && strlen(output_base_name) == 1
           && * output_base_name == '_' ) {
          free( output_base_name );
          char buffer[MAX_NAME_LENGTH];
          strcpy( buffer, dot_file_name );
#ifdef DEBUG
          printf( "output special case: buffer = %s, dot_file_name = %s\n",
                  buffer, dot_file_name );
#endif
          char * base_name_ptr = base_name( buffer );
#ifdef DEBUG
          printf( "output special case: buffer = %s, base = %s\n",
                  buffer, base_name_ptr );
#endif
          output_base_name
              = (char *) calloc( strlen(base_name_ptr) + 1, sizeof(char) );
          strcpy( output_base_name, base_name_ptr ); 
      } // end, produce output

      // read graph
      /**
       * @todo use streams instead of names
       */
      readDotAndOrd( dot_file_name, ord_file_name );
  } // end, dot and ord input
  else if ( argc == 1 ) {
      char * sgf_file_name = argv[0];
      FILE * sgf_file = fopen(sgf_file_name, "r");
      readSgf(sgf_file);
      fclose(sgf_file);
  }
  else {
      printf("Wrong number of filenames (%d)\n", argc);
      printf("Need either one sgf file or a dot and ord file\n");
      printUsage();
      exit(EXIT_FAILURE);
  }

  // create list of favored edges if appropriate
  // do the allocations unconditionally to avoid having to check for
  // 'favored_edges' everywhere
  //
  // @todo this is a test version only - treats all edges along paths
  // emanating from middle node of middle layer as favored
  /**
   * @todo this part is not likely to be useful
   */
  initPriorityEdges();
  if ( favored_edges ) {
      Layerptr middle_layer = layers[ number_of_layers / 2 ];
      int middle_node_position = middle_layer->number_of_nodes / 2;
      Nodeptr middle_node = middle_layer->nodes[ middle_node_position ];
      createFanoutList( middle_node );
      // create an actual file name and graph name here when the test version
      // is successful
      char file_name_buffer[MAX_NAME_LENGTH];
      char graph_name_buffer[MAX_NAME_LENGTH];
      char comment_buffer[MAX_NAME_LENGTH];
      createFavoredEdgeInfo( 
                            file_name_buffer,
                            graph_name_buffer,
                            comment_buffer
                             );
      writeDot( file_name_buffer, graph_name_buffer, comment_buffer,
                favoredEdges(), numberOfFavoredEdges() );
  }

  print_graph_statistics( stdout );

  initCrossings();
  initChannels();
  init_crossing_stats();
  updateAllCrossings();
  capture_beginning_stats();

  // set up structures for saving layer orders of best solutions so far
  // (these are updated as appropriate in heuristics.c)
  best_crossings_order = (Orderptr) calloc( 1, sizeof(struct order_struct) ); 
  init_order( best_crossings_order );

  best_edge_crossings_order
    = (Orderptr) calloc( 1, sizeof(struct order_struct) );
  init_order( best_edge_crossings_order );

  best_total_stretch_order
    = (Orderptr) calloc( 1, sizeof(struct order_struct) );
  init_order( best_total_stretch_order );

  best_bottleneck_stretch_order
    = (Orderptr) calloc( 1, sizeof(struct order_struct) );
  init_order( best_bottleneck_stretch_order );

  best_favored_crossings_order
    = (Orderptr) calloc( 1, sizeof(struct order_struct) ); 
  init_order( best_favored_crossings_order );

  // start the clock
  start_time = getUserSeconds();
#ifdef DEBUG
  printf( "start_time = %f\n", start_time );
#endif

  runPreprocessor();
  updateAllCrossings();
  capture_preprocessing_stats();
#ifdef DEBUG
  printf( "after preprocessor, runtime = %f\n", RUNTIME );
#endif

  // end of "iteration 0"
  end_of_iteration();
  runHeuristic();
  capture_heuristic_stats();
#ifdef DEBUG
  printf( "after heuristic, runtime = %f\n", RUNTIME );
#endif

  if ( produce_output ) {
    // write ordering after heuristic, before post-processing
    restore_order( best_crossings_order );
    createOrdFileName( output_file_name, "" );
    writeOrd( output_file_name );
  }

  if ( do_post_processing ) {
    restore_order( best_crossings_order );
    updateAllCrossings();
    swapping();

    if ( produce_output ) {
      // write file with best total crossings order after post-processing
      createOrdFileName( output_file_name, "-post" );
      writeOrd( output_file_name );
    }
  }

  capture_post_processing_stats();

#ifdef DEBUG
  updateAllCrossings();
  printf("best order restored at end, crossings = %d\n", numberOfCrossings() );
#endif

  // write file with best order for edge crossings
  if ( produce_output ) {
      // write file with best max edge order after overall
      restore_order( best_edge_crossings_order );
      createOrdFileName( output_file_name, "-edge" );
      writeOrd( output_file_name );
  }

  if ( produce_output ) {
      // write file with best stretch order overall
      restore_order( best_total_stretch_order );
      createOrdFileName( output_file_name, "-stretch" );
      writeOrd( output_file_name );
  }

  if ( produce_output ) {
      // write file with best stretch order overall
      restore_order( best_bottleneck_stretch_order );
      createOrdFileName( output_file_name, "-bs" );
      writeOrd( output_file_name );
  }

#ifdef FAVORED
  // write file with best order for favored edge crossings
  restore_order( best_favored_crossings_order );
  createOrdFileName( output_file_name, "-favored" );
  writeOrd( output_file_name );
#endif

  print_run_statistics( stdout );

  // deallocate all order structures
  cleanup_order( best_crossings_order );
  free( best_crossings_order );
  cleanup_order( best_edge_crossings_order );
  free( best_edge_crossings_order );
  cleanup_order( best_total_stretch_order );
  free( best_total_stretch_order );
  cleanup_order( best_bottleneck_stretch_order );
  free( best_bottleneck_stretch_order );
#ifdef FAVORED
  cleanup_order( best_favored_crossings_order );
  free( best_favored_crossings_order );
#endif

  /**
   * @todo Need functions to deallocate everything (in the appropriate modules)
   */
  return EXIT_SUCCESS;
}

/*  [Last modified: 2020 12 19 at 18:48:19 GMT] */

/* the line below is to ensure that this file gets benignly modified via
   'make version' */

/*xxxxxxxxxx*/
