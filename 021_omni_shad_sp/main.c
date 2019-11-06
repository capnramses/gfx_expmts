//
// single pass omni-direcitonal shadow mapping for point lights
// author: anton gerdelan <gerdela@scss.tcd.ie>
// first v. 30 nov 2015
//

#include "apg_maths.h"
#include "util.h"

#define SM_WIDTH 2048
#define SM_HEIGHT 2048

enum Face_Dir {FWD = 0, LFT, RGT, BCK, UP, DWN};
typedef enum Face_Dir Face_Dir;

struct Light {mat4 Vs[6], P;};
typedef struct Light Light;

struct Cam {mat4 V, P; float hdg; bool dirty; vec3 pos; vec4 fwd;};
typedef struct Cam Cam;

void _init_light (Light* l) {
	vec3 cp = vec3_from_3f (0.0f, 0.0f, 0.0f);
	l->Vs[FWD] = look_at (cp, vec3_from_3f (0.0f, 0.0f, -1.0f),
		vec3_from_3f (0.0f, 1.0f, 0.0f));
	l->Vs[LFT] = look_at (cp, vec3_from_3f (-1.0f, 0.0f, 0.0f),
		vec3_from_3f (0.0f, 1.0f, 0.0f));
	l->Vs[RGT] = look_at (cp, vec3_from_3f (1.0f, 0.0f, 0.0f),
		vec3_from_3f (0.0f, 1.0f, 0.0f));
	l->Vs[BCK] = look_at (cp, vec3_from_3f (0.0f, 0.0f, 1.0f),
		vec3_from_3f (0.0f, 1.0f, 0.0f));
	l->Vs[UP] = look_at (cp, vec3_from_3f (0.0f, 1.0f, 0.0f),
		vec3_from_3f (0.0f, 0.0f, 1.0f));
	l->Vs[DWN] = look_at (cp, vec3_from_3f (0.0f, -1.0f, 0.0f),
		vec3_from_3f (0.0f, 0.0f, -1.0f));
	l->P = perspective (45.0f, (float)SM_WIDTH / (float)SM_HEIGHT, 0.01f,
		100.0f);
}

void _init_cam (Cam* c) {
	float a = (float)VP_WIDTH / (float)VP_HEIGHT;
	c->P = perspective (67.0f, a, 0.1f, 100.0f);
	c->hdg = 0.0f;
	c->dirty = true;
	c->pos = vec3_from_3f (0.0f, 0.0f, 10.0f);
	c->fwd = vec4_from_4f (0.0f, 0.0f, -1.0f, 0.0f);
}

int main () {
	GLFWwindow* win = start_gl ();
	Light l;
	_init_light (&l);
	Cam c;
	_init_cam (&c);
	uint db_panel_vao = init_db_panel ();
	uint db_panel_sp = create_sp_from_files ("db_panel.vert", "db_panel.frag");
	vec3 object_poss[] = {
		vec3_from_3f (-10.0f, 5.0f, -10.0f),
		vec3_from_3f (0.0f, 0.0f, -10.0f),
		vec3_from_3f (10.0f, 0.0f, 0.0f),
		vec3_from_3f (5.0f, -5.0f, 10.0f),
		vec3_from_3f (0.0f, 10.0f, 1.0f),
		vec3_from_3f (2.0f, -8.0f, 0.0f)
	};
	uint cube_vao = 0;
	int cube_pc = 0;
	create_vao_from_obj ("cube.obj", &cube_vao, &cube_pc);

	uint cube_sp = create_sp_from_files ("cube.vert", "cube.frag");
	int cube_sp_V_loc = glGetUniformLocation (cube_sp, "V");
	int cube_sp_P_loc = glGetUniformLocation (cube_sp, "P");
	int cube_sp_M_loc = glGetUniformLocation (cube_sp, "M");
	printf ("cube sp locs %i,%i,%i\n", cube_sp_V_loc, cube_sp_P_loc,
		cube_sp_M_loc);
	uint bright_sp = create_sp_from_files ("bright.vert", "bright.frag");
	int bright_sp_V_loc = glGetUniformLocation (bright_sp, "V");
	int bright_sp_P_loc = glGetUniformLocation (bright_sp, "P");
	int bright_sp_M_loc = glGetUniformLocation (bright_sp, "M");

	uint depth_sp = create_sp_from_files ("depth.vert", "depth.frag");
	int depth_sp_V_loc = glGetUniformLocation (depth_sp, "V");
	int depth_sp_P_loc = glGetUniformLocation (depth_sp, "P");
	int depth_sp_M_loc = glGetUniformLocation (depth_sp, "M");

	uint shad_tex = 0;
	{
		glGenTextures (1, &shad_tex);
		glBindTexture (GL_TEXTURE_2D, shad_tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SM_WIDTH, SM_HEIGHT, 0, GL_RGBA,
			GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

	uint depth_tex = 0;
	{
		glGenTextures (1, &depth_tex);
		glBindTexture (GL_TEXTURE_2D, depth_tex);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexImage2D (GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SM_WIDTH, SM_HEIGHT, 0,
			GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
	}

	uint fb = 0;
	{
		glGenFramebuffers (1, &fb);
		glBindFramebuffer (GL_FRAMEBUFFER, fb);
		glFramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			GL_TEXTURE_2D, shad_tex, 0);
		glFramebufferTexture2D (GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
			GL_TEXTURE_2D, depth_tex, 0);
		GLenum status = glCheckFramebufferStatus (GL_FRAMEBUFFER);
		assert (status == GL_FRAMEBUFFER_COMPLETE);
		glBindFramebuffer (GL_FRAMEBUFFER, 0);
	}

	double p = glfwGetTime ();
	while (!glfwWindowShouldClose (win)) {
		double t = glfwGetTime ();
		double e = t - p;
		p = t;
		float mv_fwd_fac = 0.0f;
		if (glfwGetKey (win, GLFW_KEY_LEFT)) {
			c.hdg += 100.0f * e;
			c.dirty = true;
		}
		if (glfwGetKey (win, GLFW_KEY_RIGHT)) {
			c.hdg -= 100.0f * e;
			c.dirty = true;
		}
		if (glfwGetKey (win, GLFW_KEY_UP)) {
			mv_fwd_fac = 1.0f;
			c.dirty = true;
		}
		if (glfwGetKey (win, GLFW_KEY_DOWN)) {
			mv_fwd_fac = -1.0f;
			c.dirty = true;
		}
		if (c.dirty) {
			mat4 R = rot_y_deg_mat4 (c.hdg);
			c.fwd = mult_mat4_vec4 (R, vec4_from_4f (0.0f, 0.0f, -1.0f, 0.0f));
			vec3 mvd = mult_vec3_f (vec3_from_vec4 (c.fwd), mv_fwd_fac);
			c.pos = add_vec3_vec3 (c.pos, mvd);
			mat4 T = translate_mat4 (c.pos);
			c.V = mult_mat4_mat4 (inverse_mat4 (R), inverse_mat4 (T));
		}
		glBindFramebuffer (GL_FRAMEBUFFER, fb);
		glClearColor (0.2, 0.2, 0.2, 1.0);
		glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		// TODO use depth shaders
		// TODO manually apply projc + perspective division
		// then use one of 6 orthographic matrices to subset
		// (e.g. divide by 6 and offset by amount)
		// O[i] * P * v[i] * M * vp
		// ortho matrices could be hard-coded in Vshader
		// can remove colour texture output then and just use depth map
		// (but not if manually doing perspective division)
		glUseProgram (depth_sp);
		glViewport (0, 0, SM_WIDTH, SM_HEIGHT);
		glUniformMatrix4fv (depth_sp_P_loc, 1, GL_FALSE, l.P.m);
		// TODO send entire array
		glUniformMatrix4fv (depth_sp_V_loc, 1, GL_FALSE, l.Vs[0].m);
		{
			glBindVertexArray (cube_vao);
			for (int i = 0; i < 6; i++) {
				mat4 R = rot_y_deg_mat4 (t * 75.0f);
				mat4 T = translate_mat4 (object_poss[i]);
				mat4 M = mult_mat4_mat4 (T, R);
				glUniformMatrix4fv (depth_sp_M_loc, 1, GL_FALSE, M.m);
				glDrawArrays (GL_TRIANGLES, 0, cube_pc);
			}
		}
		glBindFramebuffer (GL_FRAMEBUFFER, 0);
		glClearColor (0.01, 0.01, 0.25, 1.0);
		glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport (0, 0, VP_WIDTH, VP_HEIGHT);
		{
			glUseProgram (cube_sp);
			glUniformMatrix4fv (cube_sp_V_loc, 1, GL_FALSE, c.V.m);
			glUniformMatrix4fv (cube_sp_P_loc, 1, GL_FALSE, c.P.m);
			glBindVertexArray (cube_vao);
			for (int i = 0; i < 6; i++) {
				mat4 R = rot_y_deg_mat4 (t * 75.0f);
				mat4 T = translate_mat4 (object_poss[i]);
				mat4 M = mult_mat4_mat4 (T, R);
				glUniformMatrix4fv (cube_sp_M_loc, 1, GL_FALSE, M.m);
				glDrawArrays (GL_TRIANGLES, 0, cube_pc);
			}			
		}
		{
			glUseProgram (bright_sp);
			glUniformMatrix4fv (bright_sp_V_loc, 1, GL_FALSE, c.V.m);
			glUniformMatrix4fv (bright_sp_P_loc, 1, GL_FALSE, c.P.m);
			glBindVertexArray (cube_vao);
			mat4 M = scale_mat4 (vec3_from_3f (0.2, 0.2, 0.2));
			glUniformMatrix4fv (bright_sp_M_loc, 1, GL_FALSE, M.m);
			glDrawArrays (GL_TRIANGLES, 0, cube_pc);
		}
		{
			glActiveTexture (GL_TEXTURE0);
			glBindTexture (GL_TEXTURE_2D, shad_tex);
			glUseProgram (db_panel_sp);
			glBindVertexArray (db_panel_vao);
			glDrawArrays (GL_TRIANGLE_STRIP, 0, 4);
		}
		glfwPollEvents ();
		glfwSwapBuffers (win);
	}
	return 0;
}