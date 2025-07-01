# Layered-Graphs
Programs, scripts and other utilities related to minimizing crossings and other objectives in layered graphs.
The supported objectives are
- total number of crossings
- bottleneck crossings: minimizing the maximum number of crossings of any edge
- total ***stretch***, as defined in Documents/2016-TR-Stallmann.pdf
- bottleneck stretch: minimizing the maximum stretch of an edge
- non-verticality, defined by Ulrik Brandes and Boris Köpf, *Fast and simple horizontal coordinate assignment.* In Graph Drawing: 9th International Symposium, pages 31–44, 2001.
- bottleneck non-verticality: minimizing the maximum non-verticality of any edge

The standard format used by all programs and scripts is `sgf` for *simple* graph format, described as follows
```
c first comment
...
c last comment
t GRAPH_NAME NUMBER_OF_NODES NUMBER_OF_EDGES NUMBER_OF_LAYERS
n ID_1 LAYER_1 POSITION_1
...
n ID_n LAYER_n POSITION_n
e SOURCE_1 TARGET_1
...
e SOURCE_m TARGET_m
```
Layer and position numbers are 0-based.
Conversion scripts to/from some other common formats are available.
The two-file dot and ord formats, used in 2001 and 2012 JEA papers, is still supported. However, for non-verticality, the position of a node on its layer may not correspond to its ordinal number, so the information in the ord file is insufficient.

Running multiple heuristics in sequence is now much simpler: the script `repeatHeuristics` allows a sequence of heuristics to be repeated arbitrarily many times.

Documentation for the scripts is in `scripts/0-index.html`.

Developer documentation for the source code can be obtained by running `doxygen` in the `src` directory.

The ilp directory contains scripts for creating CPLEX LP format integer/quadratic programs for combinations of the various objectives. In order to be able to generate graphs from CPLEX output, one must use the CPLEX wrapper in
`https://github.com/mfms-ncsu/CPX-ILP` and run `cplex_ilp` with the `-solution` option

For displaying small to medium size graphs, you may use Galant after conversion to `graphml` via the `sgf2layered_graphml.py` script in the `scripts` directory. See
`https://github.com/mfms-ncsu/galant`
