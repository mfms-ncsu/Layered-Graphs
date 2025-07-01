# makefile - for programs related to edge crossing minimization

CC=gcc
DFLAGS = 
OFLAGS = -O3
CFLAGS = -c -Wall -g -std=c99 $(OFLAGS) $(DFLAGS)
#CLIBS  = -lm -lgomp
CLIBS = -lm

# all functional programs
PROGRAMS = minimization verticality_adjustment_test create_random_dag add_edges dot_and_ord_to_sgf sgf_to_dot_and_ord

# various test programs
TESTS = graph_input_test crossings_test verticality_adjustment_test

# object files common to all heuristics
OBJECTS = sifting.o dfs.o gbfs.o sorting.o heuristics.o barycenter.o crossings.o\
	crossing_utilities.o graph_io.o dot.o ord.o sgf.o hash.o Statistics.o stats.o\
	positions.o swap.o median.o channel.o stretch.o timing.o random.o verticality.o dynamic_programming.o

# object files used by programs that generate random instances
CREATION_OBJECTS = check_edge_duplication.o random.o random_dag.o random_tree.o graph_io.o Statistics.o dot.o ord.o sgf.o hash.o

# header files common to all heuristics; also make sure recompilation takes
# place if there are changes to the makefile
HEADERS = makefile defs.h constants.h crossings.h graph.h graph_io.h dot.h ord.h\
	crossing_utilities.h heuristics.h barycenter.h sorting.h dfs.h gbfs.h sifting.h\
	Statistics.h stats.h positions.h swap.h median.h timing.h\
	random.h channel.h stretch.h verticality.h dynamic_programming.h makefile

# headers used by programs that generate random instances
CREATION_HEADERS = check_edge_duplication.h graph.h graph_io.h random_tree.h random_dag.h hash.h defs.h constants.h dot.h ord.h Statistics.h

.SUFFIXES: .c
.c.o: ; $(CC) $(CFLAGS) $*.c

all: $(PROGRAMS)

tests: $(TESTS)

minimization: main.o $(OBJECTS)\
; $(CC) $(DFLAGS) main.o $(OBJECTS) $(CLIBS) -o minimization

create_random_dag: create_random_dag.o $(CREATION_OBJECTS)\
; $(CC) $(OFLAGS) create_random_dag.o $(CREATION_OBJECTS) -lm -o create_random_dag

add_edges: add_edges.o $(CREATION_OBJECTS)\
; $(CC) $(OFLAGS) add_edges.o $(CREATION_OBJECTS) -lm -o add_edges

dot_and_ord_to_sgf: dot_and_ord_to_sgf.o graph_io.o dot.o ord.o hash.o sgf.o \
; $(CC) $(OFLAGS) dot_and_ord_to_sgf.o dot.o graph_io.o ord.o sgf.o hash.o -o dot_and_ord_to_sgf 

sgf_to_dot_and_ord: sgf_to_dot_and_ord.o graph_io.o dot.o ord.o hash.o sgf.o \
; $(CC) $(OFLAGS) sgf_to_dot_and_ord.o dot.o graph_io.o ord.o sgf.o hash.o -o sgf_to_dot_and_ord 

graph_input_test: graph_input_test.o dot.o ord.o hash.o sgf.o\
; $(CC) graph_input_test.o dot.o ord.o hash.o sgf.o -o graph_input_test

crossings_test: crossings_test.o sorting.o heuristics_test.o\
	 graph_io.o dot.o ord.o hash.o crossing_utilities.o\
; $(CC) crossings_test.o sorting.o heuristics_test.o\
	 graph_io.o dot.o ord.o hash.o crossing_utilities.o -o crossings_test

verticality_adjustment_test: verticality_adjustment_test.o $(OBJECTS)\
; $(CC) verticality_adjustment_test.o $(OBJECTS) -lm -o verticality_adjustment_test

# source dependencies

graph_input_test.o: graph_io.c $(HEADERS)\
; $(CC) $(CFLAGS) -DTEST graph_io.c -o graph_input_test.o

crossings_test.o: crossings.c $(HEADERS)\
; $(CC) $(CFLAGS) -DTEST crossings.c -o crossings_test.o

heuristics_test.o: heuristics.c $(HEADERS)\
; $(CC) $(CFLAGS) -DTEST heuristics.c -o heuristics_test.o

verticality_adjustment_test.o: verticality_adjustment_test.c $(HEADERS)

verticality.o: verticality.c $(HEADERS)

dynamic_programming.o: dynamic_programming.c $(HEADERS)

main.o: main.c $(HEADERS)

dot_and_ord_to_sgf.o: dot_and_ord_to_sgf.c $(HEADERS)

sgf_to_dot_and_ord.o: sgf_to_dot_and_ord.c $(HEADERS)

heuristics.o: heuristics.c $(HEADERS)

barycenter.o: barycenter.c $(HEADERS)

dfs.o: dfs.c $(HEADERS)

gbfs.o: gbfs.c $(HEADERS)

sorting.o: sorting.c $(HEADERS)

dot.o: dot.c $(HEADERS)

ord.o: ord.c $(HEADERS)

hash.o: hash.c $(HEADERS)

crossings.o: crossings.c $(HEADERS)

crossing_utilities.o: crossing_utilities.c $(HEADERS)

sifting.o: sifting.c $(HEADERS)

graph_io.o: graph_io.c $(HEADERS)

positions.o: positions.c $(HEADERS)

channel.o: channel.c $(HEADERS)

stretch.o: stretch.c $(HEADERS)

create_random_dag.o: create_random_dag.c $(CREATION_HEADERS)

add_edges.o: add_edges.c $(CREATION_HEADERS)

random_dag.o: random_dag.c $(CREATION_HEADERS)

random_tree.o: random_tree.c $(CREATION_HEADERS)

random_dag.o: random_dag.c $(CREATION_HEADERS)

check_edge_duplication.o: random_dag.c $(CREATION_HEADERS)

hash.o: hash.c $(CREATION_HEADERS)

stats.o: stats.c $(HEADERS)

Statistics.o: Statistics.c $(HEADERS)

priority_edges.o: priority_edges.c $(HEADERS)

swap.o: swap.c $(HEADERS)

median.o: median.c $(HEADERS)

timing.o: timing.c $(HEADERS)

random.o: random.c $(HEADERS)

clean: ; rm -f *.o $(PROGRAMS) *_test
