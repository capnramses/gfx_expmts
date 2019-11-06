#include "camera.h"
#include <assert.h>

Camera default_cam(){
	Camera cam;
	vec3 pos;
	pos.x = 0.25f;
	pos.y = 0.0f;
	pos.z = -0.025f;
	cam_pos(&cam, pos);
	float fovy = 67.0f;
	float aspect = 1.0f;
	float near = 0.1f;
	float far = 1000.0f;
	cam.P = perspective(fovy, aspect, near, far);
	return cam;
}

void cam_pos(Camera* cam, vec3 pos_wor){
	assert(cam);
	cam->pos_wor = pos_wor;
	vec3 targ, up;
	targ.x = 0.0;
	targ.y = 0.0;
	targ.z = 0.0;
	up.x = 0.0;
	up.y = 1.0;
	up.z = 0.0;
	cam->V = look_at(pos_wor, targ, up);
}

void cam_move(Camera* cam, vec3 moveby_wor){
	assert(cam);
	vec3 c = add_vec3_vec3(cam->pos_wor, moveby_wor);
	cam_pos(cam, c);
}
