/**
 * @file graph_io.h
 * @brief Definition of functions for reading and writing graphs.
 * @author Matt Stallmann, based on Saurabh Gupta's crossing heuristics
 * implementation.
 * @date 2008/12/19
 * $Id: graph_io.h 90 2014-08-13 20:31:25Z mfms $
 */

#include<stdbool.h>
#include<stdio.h>

#ifndef GRAPH_IO_H
#define GRAPH_IO_H

#include"graph.h"

/**
 * Reads the graph from the given dot and ord files, specified by their names.
 * Each file is read twice so that arrays can be allocated to the correct
 * size on the first pass.
 * Also initializes all graph-related data structures and global variables.
 */
void readDotAndOrd( const char * dot_file, const char * ord_file );

/**
 * Reads a graph from the given stream
 */
void readSgf(FILE * sgf_stream);

/**
 * Prints the graph in a verbose format on standard output for debugging
 * purposes. May also be used for piping to a graphical trace later.
 */
void printGraph();

/**
 * Writes the current layer orderings to an ord file with the given name.
 */
void writeOrd( const char * ord_file );

/**
 * Writes a dot file with the given name.
 * @param dot_file_name the output file name (including .dot extension)
 * @param graph_name name of graph
 * @param header_information what will go into the file before the '{' that
 * starts the list of edges (including the graph name)
 * @param edge_list array of (indices of) the edges to be written
 * @param edge_list_length length of the edge list
 */
void writeDot(const char * dot_file_name,
               const char * graph_name,
               const char * header_information,
               const Edgeptr * edge_list,
               int edge_list_length
               );

/**
 * Writes the current graph and its ordering to an sgf file with the given name.
 * @param output_stream either a pointer to a file or stdout
 */
void writeSgf(FILE * output_stream);

#endif

/*  [Last modified: 2020 12 29 at 21:43:27 GMT] */
