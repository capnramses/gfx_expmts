//
// Binary Space Partitioning Demo
// First v 20 Oct 2016  - Anton Gerdelan <gerdela@scss.tcd.ie>
// Complete rewrite 26 Oct 2016
//
// Summary of tree generation algorithm:
// 1. pick any line/wall/polygon to be 'root' node
// 2. create 2 child lists off root; infront and behind
// 3. compare all other lines to root based on their position vs.
//    root's normal. Put them into the lists.
// 4. process infront then behind list: choose a root node and recurse
//    until lists are empty and only nodes remain
//
// Assumptions:
// * I'm not actually going to load a map file (yet) - just hard-code some walls
// * A proper BSP algorithm will split larger walls that are both in-front and
// behind
//   some root wall. I won't bother with this yet - I'll just default to in-front in
//   this case.
//
// Compile:
// gcc -o bsp main.c -std=c99 -lm
//

#include <stdio.h>	 //NULL and printf
#include <assert.h>	//assert
#include <stdlib.h>	//malloc
#include <stdbool.h> //bool data type (only need this in C99)
#include <math.h>		 //sqrt

// Map: from -10 to 10 on each axis
//-10    0    10
//          W   //
// x     x-----x// -10
// |  Z  |////////
// |     x-----x//   0
// |        V  |//
// x           x//  10     V, W, Z are our camera test positions
//              //

//
// should create BSP tree:
//
//             0
//            / \
//           1
//          / \
//             2
//            / \
//               3
//              / \
//             4
//

// a wall as part of a 2d map
typedef struct Wall {
	float start_x, start_y;
	float end_x, end_y;
} Wall;

// global array of walls in map - this a constant array. probably you would
// load these from a file into a dynamic array instead
// i'm using an 'initialiser list' to define each wall in the array
Wall g_walls[5] = {
	{.start_x = -10.0f, .start_y = 10.0f, .end_x = -10.0f, .end_y = -10.0f }, // left
																																						// wall
	{.start_x = 10.0f, .start_y = -10.0f, .end_x = 0.0f, .end_y = -10.0f },		// top
																																						// right
	{.start_x = 0.0f, .start_y = -10.0f, .end_x = 0.0f, .end_y = 0.0f }, // linked to
																																			 // top right
	{.start_x = 0.0f, .start_y = 0.0f, .end_x = 10.0f, .end_y = 0.0f },	// linked to
																																			 // bottom
																																			 // right
	{.start_x = 10.0f, .start_y = 0.0f, .end_x = 10.0f, .end_y = 10.0f } // bottom
																																			 // right wall
};
int g_num_walls = 5;

// a node in the BSP tree
// declared ahead of struct because it will have node pointers
typedef struct BSP_Node BSP_Node;
struct BSP_Node {
	int wall_index;
	BSP_Node *ahead_ptr;
	BSP_Node *behind_ptr;
};

bool is_point_ahead_of( float x, float y, int id, int root_wall_index );
BSP_Node *create_bsp( int *walls_list, int num_walls );
void traverse_BSP_tree( BSP_Node *current_node, float cam_x, float cam_y );

int g_nodes_in_tree;

int main() {

	// create initial list of indices to walls (could also have copied the
	// walls around directly)
	int *walls_list = (int *)malloc( sizeof( int ) * g_num_walls );
	for ( int i = 0; i < g_num_walls; i++ ) {
		walls_list[i] = i;
	}

	// build the BSP tree from the list of walls
	BSP_Node *root = create_bsp( walls_list, g_num_walls );

	printf( "BSP tree created with %i nodes\n", g_nodes_in_tree );

	// test the drawing code
	printf("\n----Drawing from V:-----\n");
	traverse_BSP_tree(root, 5.0f, 5.0f);

	printf("\n----Drawing from W:-----\n");
	traverse_BSP_tree(root, 5.0f, -15.0f);

	printf("\n----Drawing from Z:-----\n");
	traverse_BSP_tree(root, -5.0f, -5.0f);

	return 0;
}

// draw entire scene using the painter's algorithm
// (background to foreground order)
// input is the camera location (x,y)
void traverse_BSP_tree( BSP_Node *current_node, float cam_x, float cam_y ) {
	if( !current_node ) {
		return; // nothing here, stop recursion
	}

	// 1. if current node is leaf - render current node. return
	if ( !current_node->ahead_ptr && !current_node->behind_ptr ) {
		// TODO draw this node
		printf( "draw wall %i\n", current_node->wall_index );
		return; // break recursion
	}

	// 2. if viewing location is in front of current node
	if ( is_point_ahead_of( cam_x, cam_y, -1, current_node->wall_index ) ) {
		// 2.1 render child BSP tree BEHIND current node
		traverse_BSP_tree( current_node->behind_ptr, cam_x, cam_y );
		// 2.2. render current node
		printf( "draw wall %i\n", current_node->wall_index );
		// 2.3 render child BSP tree IN FRONT of current node
		traverse_BSP_tree( current_node->ahead_ptr, cam_x, cam_y );

		// 3. otherwise if viewing location is behind current node
	} else {
		// 2.1 render BSP tree IN FRONT current node
		traverse_BSP_tree( current_node->ahead_ptr, cam_x, cam_y );
		// 2.2 render current node
		printf( "draw wall %i\n", current_node->wall_index );
		// 2.3 render BSP tree BEHIND current node
		traverse_BSP_tree( current_node->behind_ptr, cam_x, cam_y );
	}
	// 4. exact match with node is unrealistic scenario -- assume (2)
}

// returns true if a 2d point x,y is in front of another wall based on normal
// id is just for printing the number of a wall if it is the point
// TODO improve maths by using point vs plane formula instead of wall x,y average
bool is_point_ahead_of( float x, float y, int id, int root_wall_index ) {
	float root_x_avg =
		0.5f * ( g_walls[root_wall_index].end_x + g_walls[root_wall_index].start_x );
	float root_y_avg =
		0.5f * ( g_walls[root_wall_index].end_y + g_walls[root_wall_index].start_y );

	// get 2d distance vector from root to candidate
	float x_dist = x - root_x_avg;
	float y_dist = y - root_y_avg;
	// normalise the distance vector into a direction vector (make it length 1)
	float length = sqrt( x_dist * x_dist + y_dist * y_dist );
	// avoid divide by 0 error
	if ( length < 0.01 ) {
		fprintf( stderr,
						 "WARNING: two walls are on top of each other! - distance 0\n" );
		return false;
	}
	float x_dir = x_dist / length;
	float y_dir = y_dist / length;

	// work out normal of root --i'll assume 'forward' is to the right when end is at
	// the top:
	//
	//    start_x,y
	//        |
	//        |-> normal            end_x,y-----------start_x,y
	//        |                                 |
	//     end_x,y                           normal
	//

	// find direction of own line first
	float x_own_dist =
		g_walls[root_wall_index].end_x - g_walls[root_wall_index].start_x;
	float y_own_dist =
		g_walls[root_wall_index].end_y - g_walls[root_wall_index].start_y;
	float own_length = sqrt( x_own_dist * x_own_dist + y_own_dist * y_own_dist );
	float normal_x = -( y_own_dist / own_length );
	float normal_y = ( x_own_dist / own_length );

	// do dot product of root's normal with the direction vector
	float dot_prod = x_dir * normal_x + y_dir * normal_y;

	// if >= 0 then vectors are within 90 degrees of each other so it's ahead,
	if ( dot_prod > 0.0f ) {
		printf( "dot prod = %f .: %i is in front of %i\n", dot_prod, id,
						root_wall_index );
		return true;
	}
	// otherwise behind
	printf( "dot prod = %f .: %i is BEHIND %i\n", dot_prod, id, root_wall_index );
	return false;
}

// recursive function to build a tree from a list of wall indices
// delcared ahead of definition because it's recursive
// returns pointer to root node for tree (or sub-tree) or NULL
BSP_Node *create_bsp( int *walls_list, int num_walls ) {
	// stop recursion if list is empty
	if ( num_walls < 1 ) {
		return NULL;
	}
	// deliberately crash with line number message if pointer is NULL to let me know I
	// made a coding mistake
	assert( walls_list );

	// allocate heap memory for new node
	BSP_Node *node = (BSP_Node *)malloc( sizeof( BSP_Node ) );
	// i'll just always use the first wall in the input list as the
	// new root wall
	node->wall_index = walls_list[0];
	g_nodes_in_tree++;

	// i allocate more memory than I'll need here to simplify things
	int *ahead_list = (int *)malloc( sizeof( int ) * num_walls );
	int *behind_list = (int *)malloc( sizeof( int ) * num_walls );

	// sort input list into 2 new output lists; behind and ahead
	int num_ahead_walls = 0, num_behind_walls = 0;
	// starting at 1 because 0 is our root wall for this subtree
	for ( int i = 1; i < num_walls; i++ ) {
		// work out mid-point of candidate wall and root wall
		// note: a proper implementation would check if the entire wall is ahead or
		// behind
		// the line created by the root wall and split any wall that crosses over
		// maybe i'll do that later!
		float candidate_x_avg =
			0.5f * ( g_walls[walls_list[i]].end_x + g_walls[walls_list[i]].start_x );
		float candidate_y_avg =
			0.5f * ( g_walls[walls_list[i]].end_y + g_walls[walls_list[i]].start_y );
		if ( is_point_ahead_of( candidate_x_avg, candidate_y_avg, walls_list[i],
														node->wall_index ) ) {
			ahead_list[num_ahead_walls++] = walls_list[i];
		} else {
			behind_list[num_behind_walls++] = walls_list[i];
		}
	}

	// recurse for both childen of node - each gets its own sub-list
	node->ahead_ptr = create_bsp( ahead_list, num_ahead_walls );
	node->behind_ptr = create_bsp( behind_list, num_behind_walls );

	// free memory from lists
	free( ahead_list );
	free( behind_list );

	// return root of this sub-tree
	return node;
}