#pragma once
#include "linmath.h"

typedef struct Camera{
	mat4 P, V;
	vec3 pos_wor;
} Camera;

Camera default_cam();

void cam_pos(Camera* cam, vec3 pos_wor);

void cam_move(Camera* cam, vec3 moveby_wor);
