#pragma once

#define CRAPPY_LAPTOP

#include <stdio.h>	 //NULL and printf
#include <assert.h>	//assert
#include <stdlib.h>	//malloc
#include <stdbool.h> //bool data type (only need this in C99)
#include <math.h>		 //sqrt

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
bool draw_wall(float start_x, float start_y, float end_x, float end_y, int unique_id);

extern int g_num_walls;
extern int g_nodes_in_tree;
