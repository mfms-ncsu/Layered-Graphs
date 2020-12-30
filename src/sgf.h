/**
 * @file sgf.h
 * @brief
 * Module for reading and writing files in .sgf format
 * @author Matt Stallmann
 * @date 2019/12/19
 */

/**
 * sgf format is as follows (blank lines are ignored):
 *    c comment line 1
 *    ...
 *    c comment line k
 *
 *    t graph_name nodes edges layers
 *
 *    n id_1 layer_1 position_1
 *    n id_2 layer_2 position_2
 *    ...
 *    n id_n layer_n position_n
 *
 *    e source_1 target_1
 *    ...
 *    e source_m target_m
 */

#ifndef SGF_H
#define SGF_H

#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>

/**
 * Reads a graph in sgf format from the given stream
 */
void readSgf(FILE * sgf_stream);

/**
 * Writes the current graph and its ordering to an sgf file with the given name.
 * @param output_stream either a pointer to a file or stdout
 */
void writeSgf(FILE * output_stream);

#endif

/*  [Last modified: 2020 12 30 at 15:29:08 GMT] */
