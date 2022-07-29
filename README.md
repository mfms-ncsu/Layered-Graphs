# Layered-Graphs
Programs, scripts and other utilities related to minimizing crossings (and other objectives) in layered graphs. Instructions for compilation are in file `INSTALLATION.txt`
The programs use two input file formats:
1. a `.dot` file whose format is that used by `GraphViz` combined with an `.ord` file that specifies the order of nodes on each layer; or
2. a single `.sgf` file that captures both the adjancency and the layering information - the format is described in comments in the relevant programs and scripts
A variety of scripts are provided to facilitate translation from other formats.
Documentation for the scripts is in
`scripts/0_scripts.html`
Developer documentation for the source code can be obtained by running `doxygen` in the `src` directory.

The java utilities are used for layer assignment (breadth-first) when a graph is not already layered. Do `javac *.java` in the `java-utilities` directory to compile all of them.

`java-utilities/00_dag_display_process.txt` describes how to display a large layered graph.
For displaying smaller graphs, we suggest using Galant after conversion to `graphml` (scripts available in `scripts` directory). See
`https://github.com/mfms-ncsu/galant`
