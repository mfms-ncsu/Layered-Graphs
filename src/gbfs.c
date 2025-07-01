/**
 * @file gbfs.c
 * @brief implementation of guided breadth-first search
 *        see the 2001 JEA paper by Stallmann et al.
 * @author Matthias Stallmann
 * @date 2025-01-28
 */

#include "graph.h"
#include "sorting.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

/**
 * @brief Queue used for both breadth-first searches
 */
static Nodeptr * node_queue;

/**
 * @brief front and rear indexes of the queue:
 *         rear = location at which next item is to be pushed,
 *         front = location at which next node should be pulled
 */
static int front = 0;
static int rear = 0;

/**
 * @brief list of leaves
 */
static Nodeptr * leaves;
static int num_leaves = 0;

/**
 * @brief in bfs, a node needs to be marked when it is pushed to avoid being pushed again
 */
static void push(Nodeptr x) {
    if ( x->marked ) return;
    x->marked = true;
    node_queue[rear++] = x;
}

static Nodeptr pull(void) {
    return node_queue[front++];
}

static bool is_empty(void) {
    return front == rear;
}

static void init_queue(void) {
    front = rear = 0;
}

static void clear_marks(void) {
    for ( int i = 0; i < number_of_nodes; i++ ) {
        master_node_list[i]->marked = false;
    }
}

/**
 * @brief list of nodes adjacent to a given node
 *        these need to be sorted in a special way during the second, main bfs
 */
static Nodeptr * neighbors;
static int num_neighbors;

// functions for debug printing
#ifdef DEBUG
static void print_queue(void) {
    fprintf(stderr, "queue = [");
    for ( int i = front; i < rear; i++ ) {
        fprintf(stderr, " %d(%d)", node_queue[i]->id, node_queue[i]->distance);
    }
    fprintf(stderr, " ]\n");
}

static void print_leaves(void) {
    fprintf(stderr, "leaves = [");
    for ( int i = 0; i < num_leaves; i++ ) {
        fprintf(stderr, " %d(%d)", leaves[i]->id, leaves[i]->distance);
    }
    fprintf(stderr, " ]\n");
}

static void print_neighbors(void) {
    fprintf(stderr, "neighbors = [");
    for ( int i = 0; i < num_neighbors; i++ ) {
        fprintf(stderr, " %d", neighbors[i]->id);
    }
    fprintf(stderr, " ]\n");
}

static void print_depths(void) {
    fprintf(stderr, "depths = {");
    for ( int i = 0; i < number_of_nodes; i++ ) {
        fprintf(stderr, " %d[%d]", master_node_list[i]->id, master_node_list[i]->depth);
    }
    fprintf(stderr, " }\n");
}

/**
 * @brief prints weights of all the nodes
 */
void print_weights(void) {
    fprintf(stderr, "weights = {");
    for ( int i = 0; i < number_of_nodes; i++ ) {
        fprintf(stderr, " %d[%1.0f]", master_node_list[i]->id, master_node_list[i]->weight);
    }
    fprintf(stderr, " }\n");
}
#endif

static void init_gbfs(void) {
    node_queue = (Nodeptr *) calloc(number_of_nodes, sizeof(Nodeptr));
    leaves = (Nodeptr *) calloc(number_of_nodes, sizeof(Nodeptr));
    neighbors = (Nodeptr *) calloc(number_of_nodes, sizeof(Nodeptr));
}

static void cleanup(void) {
    free(node_queue);
    free(leaves);
    free(neighbors);
}

/**
 * Visits the nodes to which the given node is adjacent on the next higher layer
 * In this context, visit means
 *  - check if they're marked and skip if so
 *  - set their distance to 1 + the node's distance
 *  - put them on the queue
 * @return true if any nodes have been added to the queue 
 */
static bool visit_upper_edges(Nodeptr node) {
#ifdef DEBUG
    fprintf(stderr, "-> visit_upper_edges(%d), ", node->id);
    print_queue();
#endif
  int edge_pos = node->up_degree - 1;
  bool nodes_added = false;
  for( ; edge_pos >= 0; edge_pos-- ) {
      Nodeptr adjacent_node = node->up_edges[edge_pos]->up_node;
      if ( adjacent_node->marked ) continue;
      nodes_added = true;
      adjacent_node->distance = node->distance + 1;
      push(adjacent_node);
    }
#ifdef DEBUG
    fprintf(stderr, "<- visit_upper_edges, nodes_added = %d, ", nodes_added);
    print_queue();
#endif
    return nodes_added;
}

/**
 * Visits the nodes to which the given node is adjacent on the next lower layer
 * In this context, visit means
 *  - check if they're marked and skip if so
 *  - set their distance to 1 + the node's distance
 *  - put them on the queue
 * @return true if any nodes have been added to the queue 
 */
static bool visit_lower_edges(Nodeptr node)
{
#ifdef DEBUG
    fprintf(stderr, "-> visit_lower_edges(%d), ", node->id);
    print_queue();
#endif
  int edge_pos = 0;
  bool nodes_added = false;
  for( ; edge_pos < node->down_degree; edge_pos++ ) {
      Nodeptr adjacent_node = node->down_edges[edge_pos]->down_node;
      if ( adjacent_node->marked ) continue;
      nodes_added = true;
      adjacent_node->distance = node->distance + 1;
      push(adjacent_node);
    }
#ifdef DEBUG
    fprintf(stderr, "<- visit_lower_edges, nodes_added = %d, ", nodes_added);
    print_queue();
#endif
    return nodes_added;
}

/**
 * @brief does a breadth-first search from the start_node and sets a distance for each node,
 *        i.e., length of path in number of edges
 * @pre the graph is connected
 */
static void set_distances(Nodeptr start_node) {
#ifdef DEBUG
    fprintf(stderr, "-> set_distances(%d)\n", start_node->id);
#endif
    clear_marks();
    start_node->distance = 0;
    push(start_node);
    do {
        Nodeptr next = pull();
        bool has_upper_neighbors = visit_upper_edges(next);
        bool has_lower_neighbors = visit_lower_edges(next);
        if ( ! has_upper_neighbors && ! has_lower_neighbors ) {
            leaves[num_leaves++] = next;
        }
    } while ( ! is_empty() );
#ifdef DEBUG
    fprintf(stderr, "<- set_distances, "); print_leaves();
#endif
}

/**
 * @return the maximum depth among upper neighbors of the node
 *  Note: since we traverse the queue in reverse, depths of neighbors are set
 */
static int max_upper_depth(Nodeptr node) {
#ifdef DEBUG
    fprintf(stderr, "-> max_upper_depth(%d)\n", node->id);
#endif  
  int max_depth = -1;
  for( int edge_pos = 0; edge_pos < node->up_degree; edge_pos++ ) {
    Nodeptr adjacent_node = node->up_edges[edge_pos]->up_node;
    if ( adjacent_node->depth > max_depth ) max_depth = adjacent_node->depth;
  }
#ifdef DEBUG
    fprintf(stderr, "<- max_upper_depth, max_depth = %d\n", max_depth);
#endif
  return max_depth;
}

/**
 * @return the maximum depth among upper neighbors of the node
 *  Note: since we traverse the queue in reverse, depths of neighbors are set
 */
static int max_lower_depth(Nodeptr node) {
#ifdef DEBUG
    fprintf(stderr, "-> max_lower_depth(%d)\n", node->id);
#endif  
  int max_depth = -1;
  for( int edge_pos = 0; edge_pos < node->down_degree; edge_pos++ ) {
    Nodeptr adjacent_node = node->down_edges[edge_pos]->down_node;
    if ( adjacent_node->depth > max_depth ) max_depth = adjacent_node->depth;
  }
#ifdef DEBUG
    fprintf(stderr, "<- max_lower_depth, max_depth = %d\n", max_depth);
#endif
  return max_depth;
}

/**
 * @brief computes and sets the depth of the node
 */
static void compute_depth(Nodeptr node) {
#ifdef DEBUG
    fprintf(stderr, "-> compute_depth(%d)\n", node->id);
#endif
    int max_up_depth = max_upper_depth(node);
    int max_down_depth = max_lower_depth(node);
    node->depth = max_up_depth > max_down_depth ? max_up_depth : max_down_depth;
#ifdef DEBUG
    fprintf(stderr, "<- compute_depth(%d), depth = %d\n", node->id, node->depth);
#endif
}

/**
 * @brief sets a depth for each node using the formula
 *      - depth of x = distance of x if x is a leaf
 *      -            = max depth(y) of any child y of x
 * The children of a node are its neighbors that already have depths.
 * Computing depth can be done by traversing the queue in reverse order
 */
static void set_depths(void) {
#ifdef DEBUG
    fprintf(stderr, "-> set_depths\n");
#endif
    // initialize depths to -1 -> depth not yet computed
    for ( int i = 0; i < number_of_nodes; i++ ) {
        master_node_list[i]->depth = -1;
    }
    // initialize the depth of leaves
    for ( int i = 0; i < num_leaves; i++ ) {
        leaves[i]->depth = leaves[i]->distance;
    }
    // consider nodes on the queue in reverse order
    for ( int i = rear - 1; i >= 0; i-- ) {
        if ( node_queue[i]->depth < 0 ) compute_depth(node_queue[i]);
    }
#ifdef DEBUG
    fprintf(stderr, "<- compute_depths, "); print_depths();
#endif
}

/**
 * @brief does the first phase of gbfs:
 *  - assigns distances from the start node to each node
 *  - assigns depth(x) = max distance of any descendant of x, or, defined recursively as
 *  - if x has no descendants, depth(x) = distance(x); otherwise depth(x) = max depth(y) for any descendant(y) of x
 * a descendant is any neighbor y of x with distance(y) > distance(x)
 * Arbitrarily start the search at the first node on the master list.
 * Assume that we're experimenting with randomly permuted instances.
 * If we use the canonical instance, this will usually be the first node on layer 0.
 */
static void first_phase(void) {
    set_distances(master_node_list[0]);
    set_depths();
}

/**
 * @brief set node weights to a large integer in case some nodes are in a separate
 * component - these will come last on their layers; usually not an issue -
 * graphs should be connected
 */
static void init_weights(void) {
    for ( int i = 0; i < number_of_nodes; i++ ) {
        master_node_list[i]->weight = INT_MAX;
    }
}

/**
 * @brief a comparison function that results in nodes sorted
 *  - descreasing by distance
 *  - and increasing by depth to break ties
 * @param ptr_i pointer to first element in comparison
 * @param ptr_j pointer to second element in comparison
 * @return an int that ensures the comparison is done correctly
 */
static int gbfs_comparison(const void * ptr_i, const void * ptr_j) {
    // convert void pointers to Nodeptr *'s and get pointers to the two nodes
    Nodeptr * entry_ptr_i = (Nodeptr *) ptr_i;
    Nodeptr * entry_ptr_j = (Nodeptr *) ptr_j;
    Nodeptr node_i = * entry_ptr_i;
    Nodeptr node_j = * entry_ptr_j;
    if ( node_i->distance > node_j->distance ) return -1;
    else if ( node_i->distance < node_j->distance ) return 1;
    else if ( node_i->depth > node_j->depth ) return 1;
    else if ( node_i->depth < node_j->depth ) return -1;
    else return 0;
}

/**
 * @brief puts the neighbors of the node into the child_list in a sorted order based on gbfs:
 *  - decreasing by distance
 *  - increasing by depth to break ties
 */
static void compute_sorted_neighbors(Nodeptr node) {
#ifdef DEBUG
    fprintf(stderr, "-> compute_sorted_neighbors(%d)\n", node->id);
#endif
    num_neighbors = 0;
    // collect the adjacent nodes, both upper and lower, keeping only the neighbors
    for ( int i = 0; i < node->up_degree; i++ ) {
        Nodeptr neighbor = node->up_edges[i]->up_node;
        neighbors[num_neighbors++] = neighbor;
    }
    for ( int i = 0; i < node->down_degree; i++ ) {
        Nodeptr neighbor = node->down_edges[i]->down_node;
        neighbors[num_neighbors++] = neighbor;
    }
    insertionSort(neighbors, num_neighbors, sizeof(Nodeptr), gbfs_comparison);
#ifdef DEBUG
    fprintf(stderr, "<- compute_sorted_neighbors(%d), ", node->id); print_neighbors();
#endif
}

/**
 * @brief does a second bfs and sets weights based on the order of appearance of the nodes
 *  - start at a node with greatest distance
 *  - traverse each adjacency list sorted by decreasing distance
 *    and, in case of ties, increasing depth
 */
static void final_bfs(void) {
    // last node on queue is guaranteed to have greatest distance
    Nodeptr start_node = node_queue[rear - 1];
#ifdef DEBUG
    fprintf(stderr, "-> final_bfs, start = %d\n", start_node->id);
#endif
    // intialize queue for second bfs
    init_queue();
    clear_marks();
    init_weights();
    push(start_node);
    int current_weight = 0;
    do {
        Nodeptr next = pull();
        next->weight = current_weight++;
        compute_sorted_neighbors(next);
        for ( int i = 0; i < num_neighbors; i++) {
            push(neighbors[i]);
        }
#ifdef DEBUG
        fprintf(stderr, "next = %d, ", next->id); print_queue();
#endif        
    } while ( ! is_empty() );
#ifdef DEBUG
    fprintf(stderr, "<- final_bfs, "); print_weights();
#endif
}

void assignGbfsWeights(void) {
    init_gbfs();
    first_phase();
    final_bfs();
    cleanup();
}
