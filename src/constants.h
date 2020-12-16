/**
 * @file constants.h
 * @brief
 * Important constants and enums
 */

#ifndef CONSTANTS_H
#define CONSTANTS_H

/**
 * maximum number of characters in various buffers (includes null terminator)
 */
#define MAX_NAME_LENGTH 512

/**
 * maximum length of a line in a .ord file during output
 */
#define LINE_LENGTH 75

/**
 * starting capacity and additional capacity for dynamic arrays
 */
#define CAPACITY_INCREMENT 32

/**
 * maximum number of digits in a node id (for sgf) + 1
 */
#define MAX_ID_SIZE 10

/**
 * Used with sorting heuristics to indicate whether weights are computed
 * based on edges above, below, or on both sides of a layer to be
 * sorted. This is referred to as 'orientation' in the thesis 
 */
typedef enum { UPWARD, DOWNWARD, BOTH } Orientation;

#endif

/*  [Last modified: 2020 12 16 at 16:56:28 GMT] */
