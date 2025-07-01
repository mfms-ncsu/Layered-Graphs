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
#include"positions.h"
#include"timing.h"
#include"random.h"
#include"verticality.h"

// definition of command-line options with default values

char * command_line = NULL;
char * objective = NULL;
char * heuristic = NULL;
char * preprocessor = NULL;
int max_iterations = INT_MAX;
int max_passes = INT_MAX;
double runtime = 0;
double max_runtime = DBL_MAX;
double start_time = 0;
enum adjust_weights_enum adjust_weights = LEFT;
enum sift_option_enum sift_option = DEGREE;
enum sifting_style_enum sifting_style = DEFAULT;
enum pareto_objective_enum pareto_objective = NO_PARETO;
bool randomize_order = false;
bool do_random_sift = false;
bool verticality_heuristic = false;
/**
 * @todo not clear which value of this option works best; stay tuned ...
 */
bool balanced_weight = false;

/**
 * The base name of the input file.
 */
static char input_base_name[MAX_NAME_LENGTH];

/**
 * The base name used for the output file
 * This may be different from input_base_name if the -w option is used
 * or the input file name does not match the graph name.
 * A warning is issued in the latter case.
 */
static char output_base_name[MAX_NAME_LENGTH];

/**
 * write_files is set when user specifies the -w option
 * write_ord_output and write_sgf_output are based on the nature
 * of the input; these are defined in graph_io.c
 */
bool write_files = false;

/**
 * if true, write the input file on stdout and exit 
 */
static bool write_and_exit = false;

/**
 * base_name_arg stores a base name given by a -w option,
 * while output_base_name is the actual base name used,
 * which will differ if the arg is "_"
 */
static char * base_name_arg = NULL;

// user specified stdin with -I option
bool stdin_requested = false;
// user specified stdout with '-O' option
// this one is made extern in defs.h so that other parts of program
// can figure out what type of output is desirable
bool write_stdout = false;

bool verbose = false;
int trace_freq = INT_MAX;

// definition of order saving structures
Positionptr best_crossings_order = NULL;
Positionptr best_edge_crossings_order = NULL;
Positionptr best_total_stretch_order = NULL;
Positionptr best_bottleneck_stretch_order = NULL;
Positionptr best_favored_crossings_order = NULL;
Positionptr best_nonverticality_order = NULL;
Positionptr best_bottleneck_verticality_order = NULL;

static bool do_post_processing = false;

static char error_message_buffer[MAX_NAME_LENGTH];

/**
 * prints an error message with a prompt to run with -H for help
 */
static void printError(const char * message) {
  fprintf(stderr, "*** Error: %s ***\n", message);
  fprintf(stderr, "+++ use -H option for a usage message +++\n");
}

/**
 * prints usage message
 *
 * @todo sort these alphabetically and make them more meaningful and/or use
 * long options
 */
static void printUsage(void) {
  fprintf(stderr, "Usage: minimization [opts] [file(s)]\n"
          " the file(s) part is\n"
          "   * missing - read from stdin and assume sgf format (only if -I is an opt)\n"
          "   * one file name - assumed to be an sgf file\n"
          "   * two file names - assumed to be a dot and an ord file\n");
  fprintf(stderr, " the opts are zero or more of the following\n" );
  fprintf(stderr,
         "  -H to print this message and exit\n"
         "  -A to write the input graph (showing crossings and verticalities) and exit\n"
         "  -I read from standard input, assume sgf format\n"
         "  -h (median | bary | mod_bary | vertical_bary | mcn | sifting | mce | mce_t | mse | mnve)\n"
         "     [main heuristic - default none]\n"
         "  -p (gbfs | dfs | mds) [preprocessing - default none]\n"
         "  -z if post processing is wanted (repeated swaps until no improvement in crossings)\n"
         "     recommended only if the objective is to minimize total crossings\n"
         "  -x do a random sift at the beginning of each pass [* has a bug *]\n"
         "       i.e., pick a random edge and put both its endpoints in random positions\n"
         "       use -R if a seed is desired\n"
         "  -i MAX_ITERATIONS [default: ensure another stopping criterion, see -a and -r]\n"
         "  -a MAX_PASSES     [default: ensure another stopping criterion, see -i and -r]\n"
         "  -r MAX_RUNTIME (seconds) [default: ensure another stopping criterion, see -i and -a]\n"
         "  -R SEED edge list, node list, or sequence of layers will be randomized\n"
         "     after each pass of mod_bary, mce, mcn, mse, sifting, etc.\n"
         "     to break ties differently when sorting; SEED is an integer seed\n"
         "  -P PARETO_OBJECTIVES (b_t | t_s | b_s | t_v | b_v) pair of objectives for Pareto optima\n"
         "      b = bottleneck, t = total, s = stretch v = verticality (default = none)\n"
         "      'bottleneck' is also known as 'min-max edge'\n"
         "  -w BASE produce file(s) with name(s) BASE-H-O.EXT,\n"
         "          where H is the heuristic(s) used, O is the objective,\n"
         "          and EXT is either sgf or ord, depending on input format\n"
         "     -w _ (underscore) means use the name of the graph as base name\n"
         "  -O (upper case oh) send output to stdout\n"
         "     if -P is used, the line with Pareto optima is appended as a comment\n"
         "  -o OBJECTIVE write best configuration for OBJECTIVE as sgf output to stdout\n"
         "          (default = configuration at program termination)\n"
         "      t = total, b = bottleneck, s = stretch, bs = bottleneck stretch, v = nonverticality\n"
         "  -s (layer | degree | random) [sifting variation - see paper]\n"
        //  "  -g (total | max) [what sifting is based on] [default: total for sifting, mcn; max for mce]\n"
        //  "      [not implemented yet]\n"
         "  -v to get verbose information about the graph\n"
         "  -t trace_freq, if trace printout is desired, 0 means only at the end of a pass, > 0 sets frequency\n"
	       "        -1 prints a message each time any of the objectives improves\n"
         );
}

static void runPreprocessor(void) {
  fprintf(stderr, "--- Running preprocessor %s\n", preprocessor);
  if ( strcmp( preprocessor, "gbfs" ) == 0 )
    guidedBreadthFirstSearch();
  else if ( strcmp( preprocessor, "dfs" ) == 0 )
    depthFirstSearch();
  else if ( strcmp( preprocessor, "mds" ) == 0 )
    middleDegreeSort();
  else {
      sprintf(error_message_buffer, "Bad preprocessor '%s'\n", preprocessor );
      printError(error_message_buffer);
      exit( EXIT_FAILURE );
    }
}

/**
 * @todo It would be nice to have a way to run two heuristics, one after the
 * other. Not really needed - can always use the output file of one as input
 * to the other
 */
static void runHeuristic(void) {
  fprintf(stderr, "=== Running heuristic %s\n", heuristic);
  if( strcmp( heuristic, "median" ) == 0 )
    median();
  else if( strcmp( heuristic, "bary" ) == 0 )
    barycenter();
  else if( strcmp( heuristic, "mod_bary" ) == 0 )
    modifiedBarycenter();
  else if( strcmp( heuristic, "vertical_bary" ) == 0 ) {
    verticality_heuristic = true;
	  verticalityBarycenter();
  }
  else if ( strcmp( heuristic, "mcn" ) == 0 )
    maximumCrossingsNode();
  else if ( strcmp( heuristic, "mce_t" ) == 0 )
    maximumCrossingsEdgeWithSifting();
  else if ( strcmp( heuristic, "sifting" ) == 0 )
    sifting();
  else if( strcmp( heuristic, "mce" ) == 0 ) {
    maximumCrossingsEdge();
  }
  else if( strcmp( heuristic, "mse" ) == 0 ) {
    maximumStretchEdge();
  }
  else if( strcmp( heuristic, "mnve" ) == 0 ) {
    verticality_heuristic = true;
    maximumNonVerticalityEdge();
  }
  else {
    sprintf(error_message_buffer, "Bad heuristic '%s'\n", heuristic);
    printError(error_message_buffer);
    exit( EXIT_FAILURE );
  }
}

/**
 * puts the command line into the given buffer
 */
void captureCommandLine(char * cmd_line_buffer, int argc, char * argv[]) {
    char local_buffer[MAX_NAME_LENGTH];
    *cmd_line_buffer = '\0';
    for ( int counter = 0; counter < argc; counter++ ) {
        sprintf(local_buffer, "%s", argv[counter]);
        if ( counter > 0 ) strcat(cmd_line_buffer, " ");
        strcat(cmd_line_buffer, local_buffer);
    }
}

/**
 * Deallocates memory allocated during input or computation
 */
void deallocateAll(void) {
  deallocateGraph();
  deallocateCrossingStructs();
  deallocateChannels();
  deallocateParetoList();
  deallocateDPMatrices();
}

/**
 * As of now, the main program does the following seqence of events -
 * -# If there are two args, treat them as a dot and ord file and read
 * -# If there is one arg, treat it as an sgf file and read
 * -# If there are no args, use standard input if
 * -# Display the attributes of the graph
 * -# Count the number of crossings.
 * -# Apply a preprocess and a heuristic (both optional) on the graph
 * -# Optionally apply a post-processor that repeatedly swaps neighboring
 * nodes until there's no more improvement
 * -# Count the number of crossings after each phase (and save the ORD files
 * for the minimum number of crossings in each phase)
 */
int main(int argc, char * argv[]) {
  fprintf(stderr, "----------- minimization, release 1.1, 2020/12/22 ----------\n");

  char cmd_line_buffer[MAX_NAME_LENGTH];
  
  captureCommandLine(cmd_line_buffer, argc, argv);
  command_line = calloc(strlen(cmd_line_buffer) + 1, sizeof(char));
  strcpy(command_line, cmd_line_buffer);
  fprintf(stderr, "+++ Starting %s\n", command_line);
  
  int seed = 0;
  int ch = -1;

  // process command-line options; these must come before the file arguments
  // note: options that have an arg are followed by : but others are
  // not
  while ( (ch = getopt(argc, argv, "Aa:c:fgHh:Ii:Oo:p:P:R:r:s:t:vw:z")) != -1) {
      switch(ch) {
        case 'H':
          printUsage();
          exit(EXIT_FAILURE);
        case 'A':
          write_and_exit = true;
          break;
        case 'I':
            stdin_requested = true;
            break;
        case 'h':
          heuristic = optarg;
          break;

        case 'p':
          preprocessor = optarg;
          break;

        case 'z':
          do_post_processing = true;
          break;

        case 'x':
          do_random_sift = true;
          break;

        case 'i':
            if ( strspn(optarg, "0123456789") != strlen(optarg) ) {
              sprintf(error_message_buffer, "Value '%s' for -i option is not an integer", optarg);
              printError(error_message_buffer);
              exit( EXIT_FAILURE );
            }
          max_iterations = atoi( optarg );
          break;

        case 'a':
            if ( strspn(optarg, "0123456789") != strlen(optarg) ) {
              sprintf(error_message_buffer,"Value '%s' for -a option is not an integer", optarg);
              printError(error_message_buffer);
              exit(EXIT_FAILURE);
            }
          max_passes = atoi( optarg );
          break;

        case 'r':
            if ( strspn(optarg, ".0123456789") != strlen(optarg) ) {
              sprintf(error_message_buffer, "Value '%s' for -r option is not a floating point number\n", optarg);
              printError(error_message_buffer);
              exit( EXIT_FAILURE );
            }
          max_runtime = atof( optarg );
          break;

        case 'R':
        /**
         * @todo there's a better way to convert to an int and check at the same time
         */
            if ( strspn(optarg, "0123456789") != strlen(optarg) ) {
              sprintf(error_message_buffer, "Value '%s' for -R option is not an integer", optarg);
              printError(error_message_buffer);
              exit( EXIT_FAILURE );
            }
          seed = atoi( optarg );
          init_genrand( seed );
          randomize_order = true;
          break;

        case 'P':
          if ( strcmp( optarg, "b_t" ) == 0 ) pareto_objective = BOTTLENECK_TOTAL;
          else if ( strcmp( optarg, "t_s" ) == 0 ) pareto_objective = TOTAL_STRETCH; 
          else if ( strcmp( optarg, "b_s" ) == 0 ) pareto_objective = BOTTLENECK_STRETCH;
          else if ( strcmp( optarg, "t_v" ) == 0 ) pareto_objective = TOTAL_VERTICAL;
          else if ( strcmp( optarg, "b_v" ) == 0 ) pareto_objective = BOTTLENECK_VERTICAL;
          else {
            sprintf(error_message_buffer, "Bad value '%s' for option -P", optarg);
            printError(error_message_buffer);
            exit( EXIT_FAILURE );
          }
          break;

        case 'O':
            write_stdout = true;
            break;
                
        case 'o':
            if ( strcmp(optarg, "t") != 0
                 && strcmp(optarg, "b") != 0
                 && strcmp(optarg, "s") != 0
                 && strcmp(optarg, "bs") != 0
	               && strcmp(optarg, "v") != 0
                 && strcmp(optarg, "bv") != 0 ) {
                sprintf(error_message_buffer, "Bad value '%s' for option -o", optarg );
                printError(error_message_buffer);
                exit(EXIT_FAILURE);
            }
            objective = calloc(strlen(optarg) + 1, sizeof(char));
            strcpy(objective, optarg);
            break;

        case 'w':
            write_files = true;
            base_name_arg = calloc(strlen(optarg) + 1, sizeof(char));
            strcpy(base_name_arg, optarg);
            break;

        case 's':
          if( strcmp( optarg, "layer" ) == 0 ) sift_option = LAYER;
          else if( strcmp( optarg, "degree" ) == 0 ) sift_option = DEGREE; 
          else if( strcmp( optarg, "random" ) == 0 ) sift_option = RANDOM;
          else {
            sprintf(error_message_buffer, "Bad value '%s' for option -s", optarg );
            printError(error_message_buffer);
            exit( EXIT_FAILURE );
          }
          break;

        case 'g':
          if( strcmp( optarg, "total" ) == 0 ) sifting_style = TOTAL;
          else if( strcmp( optarg, "max" ) == 0 ) sifting_style = MAX; 
          else {
            sprintf(error_message_buffer, "Bad value '%s' for option -g", optarg);
            printError(error_message_buffer);
            exit( EXIT_FAILURE );
          }
          break;

        case 'v':
          verbose = true;
          break;

        case 't':
          if ( strspn(optarg, "-0123456789") != strlen(optarg) ) {
            sprintf(error_message_buffer, "Value '%s' for -t option is not an integer", optarg);
            exit(EXIT_FAILURE);
          }
          trace_freq = atoi( optarg );
          break;

        default:
          sprintf(error_message_buffer, "unknown option");
          printError(error_message_buffer);
          exit(EXIT_FAILURE);
          break;

        }  /* end of switch */
    }  /* end of while */

  // make sure there is a stopping criterion
  if ( max_iterations == INT_MAX && max_passes == INT_MAX && max_runtime == DBL_MAX
      && heuristic != NULL ) {
    sprintf(error_message_buffer, "No stopping criterion specified for heuristic %s:"
                    " use -i, -a, or -r", heuristic);
    printError(error_message_buffer);
    exit(EXIT_FAILURE);
  }

  // start command line at first index after the options and get the two file
  // names: dot and ord, respectively
  argc -= optind;
  argv += optind;

  input_base_name[0] = '\0';
  if ( argc == 2 ) {
      const char * dot_file_name = argv[0];
      const char * ord_file_name = argv[1];

      // read graph
      /**
       * @todo use streams instead of names
       */
      readDotAndOrd( dot_file_name, ord_file_name );
      if ( write_files ) {
          write_ord_output = true;
      }
  } // end, dot and ord input
  else if ( argc == 1 ) {
      char * sgf_file_name = argv[0];
      getBaseName(input_base_name, sgf_file_name);
      FILE * input_stream = fopen(sgf_file_name, "r");
      if ( input_stream == NULL ) {
        sprintf(error_message_buffer, "File %s could not be opened", sgf_file_name);
        printError(error_message_buffer);
        exit(EXIT_FAILURE);
      }

      readSgf(input_stream);
      fclose(input_stream);
      if ( write_files ) {
          write_sgf_output = true;
      }
  }
  else if ( argc == 0 ) {
      if ( stdin_requested ) {
          readSgf(stdin);
          if ( write_files ) {
              write_sgf_output = true;
          }
      }
      else {
          sprintf(error_message_buffer,
           "Need to specify -I to request stdin if there are no files on command line\n"
           "    or need either one sgf file or both a dot file and an ord file");
          printError(error_message_buffer);
          exit(EXIT_FAILURE);
      }
  }
  else {
      sprintf(error_message_buffer, "Wrong number of filename arguments (%d)", argc);
      printError(error_message_buffer);
      exit(EXIT_FAILURE);
  }

  addComment(command_line, true);
  
  if ( write_files ) {
    if ( strcmp(base_name_arg, "_") == 0 ) {
      if ( strcmp(input_base_name, graph_name) != 0 ) {
         fprintf(stderr, "*** Warning: filename base %s does not match graph name %s\n",
                 input_base_name, graph_name);
         fprintf(stderr, "***          output base defaults to graph name\n");
      }
      strcpy(output_base_name, graph_name);
    }
    else {
      strcpy(output_base_name, base_name_arg);
    }
  }

  if ( ! write_stdout ) {
      print_graph_statistics( stdout );
  }

  allocateDPMatrices();
  allocateCrossingStructs();
  initChannels();
  init_crossing_stats();

  // set up structures for saving layer orders of best solutions so far
  // (these are updated as appropriate in heuristics.c)
  best_crossings_order = (Positionptr) calloc( 1, sizeof(struct position_struct) );
  init_position_struct(best_crossings_order);

  best_nonverticality_order = (Positionptr) calloc( 1, sizeof(struct position_struct) );
  init_position_struct(best_nonverticality_order);

  best_bottleneck_verticality_order = (Positionptr) calloc( 1, sizeof(struct position_struct) );
  init_position_struct(best_bottleneck_verticality_order);

  best_edge_crossings_order
    = (Positionptr) calloc( 1, sizeof(struct position_struct) );
  init_position_struct(best_edge_crossings_order);

  best_total_stretch_order
    = (Positionptr) calloc( 1, sizeof(struct position_struct) );
  init_position_struct(best_total_stretch_order);

  best_bottleneck_stretch_order
    = (Positionptr) calloc( 1, sizeof(struct position_struct) );
  init_position_struct(best_bottleneck_stretch_order);

  best_favored_crossings_order
    = (Positionptr) calloc( 1, sizeof(struct position_struct) );
  init_position_struct(best_favored_crossings_order);

  // start the clock
  start_time = getUserSeconds();
#ifdef DEBUG
  fprintf(stderr, "start_time = %f\n", start_time );
#endif

  updateAllCrossings();
  updateAllVerticality();

  if ( write_and_exit ) {
    writeGraph(stdout);
    exit(EXIT_SUCCESS);
  }

  update_best_all();
  capture_beginning_stats();

  if ( preprocessor != NULL )
    runPreprocessor();

  updateAllCrossings();
  adjustVerticalities();
  update_best_all();
  capture_preprocessing_stats();
#ifdef DEBUG
  fprintf(stderr, "after preprocessor, runtime = %f\n", RUNTIME );
#endif

  if ( heuristic != NULL )
    runHeuristic();

  capture_heuristic_stats();
#ifdef DEBUG
  fprintf(stderr,  "after heuristic, runtime = %f\n", RUNTIME );
#endif

  if ( write_files ) {
      // write ordering after heuristic, before post-processing
    fprintf(stderr, "about to restore best crossings order\n");
	  restorePositions(best_crossings_order);
    fprintf(stderr, "done restoring best crossings order\n");
    fprintf(stderr, "output base = %s\n", output_base_name);
    writeFile(output_base_name, "t");
    fprintf(stderr, "done writing file\n");
  }

  if ( do_post_processing ) {
    fprintf(stderr, "___ Running post-processor\n");
	  restorePositions(best_crossings_order);
    updateAllCrossings();
    fprintf(stderr, "before swapping., crossings = %d\n", numberOfCrossings());
    swapping();
    updateAllCrossings();
    fprintf(stderr, "after swapping., crossings = %d\n", numberOfCrossings());
//  no need for this -- crossings will improve if the swapping does anything
//    update_best_all();
    if ( write_files ) {
      writeFile(output_base_name, "post");
    }
  }

  updateAllCrossings();
  capture_post_processing_stats();

#ifdef DEBUG
  updateAllCrossings();
  fprintf(stderr, "best order restored at end, crossings = %d\n", numberOfCrossings() );
#endif

  // write file with best order for edge crossings
  if ( write_files ) {
      // write file with best max edge order after overall
      restorePositions( best_edge_crossings_order );
      writeFile(output_base_name, "b");

      // write file with best stretch order overall
      restorePositions( best_total_stretch_order );
      writeFile(output_base_name, "s");

      // write file with best bottleneck stretch order overall
      restorePositions( best_bottleneck_stretch_order );
      writeFile(output_base_name, "bs");

  	  // write file with best nonverticality
	    restorePositions( best_nonverticality_order );
	    writeFile(output_base_name,  "v" );

	    // write file with best nonverticality
	    restorePositions( best_bottleneck_verticality_order );
	    writeFile(output_base_name,  "bv" );
  }

  // write to stdout if requested; note that this is independent of
  // writing files so possible to do both
  if ( write_stdout ) {
      char runtime_buffer[MAX_NAME_LENGTH];
      sprintf(runtime_buffer, "Runtime,%4.2f", RUNTIME);
      addComment(runtime_buffer, true);
      if ( pareto_objective != NO_PARETO ) { 
          char pareto_buffer[MAX_NAME_LENGTH];
          getParetoList(pareto_buffer);
          addComment(pareto_buffer, true);
      }
      if ( objective == NULL ) objective = "_";

      if ( strcmp(objective, "t") == 0 ) {
          restorePositions(best_crossings_order);
      }
      else if ( strcmp(objective, "b") == 0 ) {
          restorePositions(best_edge_crossings_order);
      }
      else if ( strcmp(objective, "s") == 0 ) {
          restorePositions(best_total_stretch_order);
      }
      else if ( strcmp(objective, "bs") == 0 ) {
          restorePositions(best_bottleneck_stretch_order);
      }
      else if ( strcmp(objective, "v") == 0 ) {
          restorePositions(best_nonverticality_order);
      }
      else if ( strcmp(objective, "bv") == 0 ) {
          restorePositions(best_bottleneck_verticality_order);
      }
      writeSgf(stdout, "anonymous");
  }

  if ( ! write_stdout ) {
      print_run_statistics( stdout );
  }
  else {
    print_run_statistics(stderr);
  }

  // deallocate all order structures
  cleanup_position_struct( best_crossings_order );
  free( best_crossings_order );
  cleanup_position_struct( best_nonverticality_order );
  free( best_nonverticality_order );
  cleanup_position_struct( best_bottleneck_verticality_order );
  free( best_bottleneck_verticality_order );
  cleanup_position_struct( best_edge_crossings_order );
  free( best_edge_crossings_order );
  cleanup_position_struct( best_total_stretch_order );
  free( best_total_stretch_order );
  cleanup_position_struct( best_bottleneck_stretch_order );
  free( best_bottleneck_stretch_order );

  deallocateAll();

  return EXIT_SUCCESS;
}
