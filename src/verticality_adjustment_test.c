/**
 * @file verticality_adjustment_test.c
 * @brief program to try out various heuristics that adjust verticality
 *        on a single layer
 * @author Matt Stallmann
 * @date 2022/04/20
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
#include"graph_io.h"
#include"graph.h"
#include"barycenter.h"
#include"sorting.h"
#include"timing.h"
#include"verticality.h"
#include"dynamic_programming.h"

char * heuristic = NULL;
char * preprocessor = NULL;

/**
 * The base name of the input file.
 */
static char input_base_name[MAX_NAME_LENGTH];

char * command_line = NULL;
double runtime = 0;
double start_time = 0;
enum adjust_weights_enum adjust_weights = LEFT;
/**
 * @todo not clear which value of this option works best; stay tuned ...
 */
bool balanced_weight = false;

/**
 * layer on which to adjust verticality (defaults to 0)
 */
int layer_to_adjust = 0;

/**
 * base_name_arg stores a base name given by a -w option,
 * while output_base_name is the actual base name used,
 * which will differ if the arg is "_"
 */
char * base_name_arg = NULL;

bool do_random_sift = false;
bool verticality_heuristic = true;

bool verbose = false;
int trace_freq = -1;

/**
 * prints usage message
 */
static void printUsage(void)
{
  printf("Usage: verticality_adjustment_test [opts] file\n"
         " the file part is a file in sgf format\n"
         " the opts are zero or more of the following\n"
         "  -h [heuristic]\n"
         "      opt  \tdynamic programming optimal algorithm\n"
         "      pull \tgoes right to left and decrements weights when there are conflicts\n"
         "      merge\tidentifies equal weight blocks; merges if not enough room for centering\n"
         "  -L [layer] number of the layer to adjust (default = 0)\n"
         "  -v to get verbose information about the graph\n"
         );
}

/**
 * @todo Fill in later
 */
static void runVerticalityHeuristic(int layer_number)
{
  if ( heuristic == NULL )
    ;                           /* do nothing */
  else if ( strcmp( heuristic, "opt" ) == 0 ) {
      optimizeLayerVerticality(layer_number, BOTH);
  }
  else {
      printf( "Bad heuristic '%s'\n", heuristic );
      printUsage();
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
 * @todo check for anything else that needs to be deallocated
 */
void deallocateAll(void) {
  deallocateGraph();
  deallocateDPMatrices();
}

/**
 * reads the file and uses the chosen heuristic to adjust verticality
 * on the specific layer
 * writes an sgf file that encodes the adjusted positions
 */
int main( int argc, char * argv[] ) {
  int ch = -1;
  // process command-line options; these must come before the file arguments
  // note: options that have an arg are followed by : but others are
  // not
  while ( (ch = getopt(argc, argv, "h:L:v")) != -1 ) {
      switch(ch) {
        case 'h':
          heuristic = optarg;
          break;
        case 'L':
          layer_to_adjust = atoi(optarg);
        case 'v':
          verbose = true;
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
  if ( argc != 1 ) {
      printf("** Error: Wrong number of filename arguments (%d)\n", argc);
      printUsage();
      exit(EXIT_FAILURE);
  }
  char * sgf_file_name = argv[0];
  FILE * input_stream = fopen(sgf_file_name, "r");
  if( input_stream == NULL ) {
      fprintf(stderr, "*** FATAL ERROR: Unable to open file %s for input\n", sgf_file_name);
      exit(EXIT_FAILURE);
  }
  readSgf(input_stream);
  fclose(input_stream);

  getBaseName(input_base_name, sgf_file_name);
  if ( strlen(input_base_name) > 0
       && strcmp(input_base_name, graph_name) != 0 ) {
         fprintf(stderr, "*** Warning: filename base %s does not match graph name %s\n",
                 input_base_name, graph_name);
         fprintf(stderr, "***          output base defaults to graph name\n");
       }

  if ( layer_to_adjust >= number_of_layers ) {
    printf("** Error: Layer chosen by -L is %d, but max layer number is %d\n",
          layer_to_adjust, number_of_layers - 1);
    printUsage();
    exit(EXIT_FAILURE);
  }
  unsigned long layer_verticality = updateLayerVerticality(layer_to_adjust);
  printf("initially\t%15lu\n", layer_verticality);

  // do one iteration of barycenter for the specified layer
  barycenterWeights(layer_to_adjust, BOTH);
  layerSort(layer_to_adjust);

  layer_verticality = updateLayerVerticality(layer_to_adjust);
  printf("after_sort\t%15lu\n", layer_verticality);

  allocateDPMatrices();

  double start_time = getUserSeconds();
  runVerticalityHeuristic(layer_to_adjust);
  double runtime = getUserSeconds() - start_time;


  layer_verticality = updateLayerVerticality(layer_to_adjust);
  printf("finally   \t%15lu\n", layer_verticality);

  printf("runtime\t%f\n", runtime);

  // 4 for .sgf, 2 for -v tag, and 1 for null terminator
  char * output_file_name = calloc(strlen(graph_name) + 7, sizeof(char));
  strcpy(output_file_name, graph_name);
  strcat(output_file_name, "-v");
  strcat(output_file_name, ".sgf");
  FILE * output_stream = fopen(output_file_name, "w");
  writeSgf(output_stream, "anonymous");
  free(output_file_name);

  deallocateAll();
  return EXIT_SUCCESS;
}

// ----- 
// The following are not used but need to be included to keep the compiler happy
// -----

Positionptr best_crossings_order = NULL;
Positionptr best_edge_crossings_order = NULL;
Positionptr best_total_stretch_order = NULL;
Positionptr best_bottleneck_stretch_order = NULL;
Positionptr best_favored_crossings_order = NULL;
Positionptr best_nonverticality_order = NULL;
Positionptr best_bottleneck_verticality_order = NULL;
int max_iterations = INT_MAX;
int max_passes = INT_MAX;
int capture_iteration = INT_MIN;
double max_runtime = DBL_MAX;
enum pareto_objective_enum pareto_objective = NO_PARETO;
bool randomize_order = false;
bool standard_termination = true;
bool write_files = false;
