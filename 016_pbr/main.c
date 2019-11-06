//
// PBR experiment in opengl
// dr anton gerdelan 21 oct 2015 gerdela at tcd dot ie
// trinity college dublin, ireland
//

#include "apg_maths.h"
#include "gl_utils.h"
#include "obj_parser.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

/* Note:
if using fixed var names in shaders
could have generic setup_shader function
that did glGetUniformLocation for all known types and recored booleans
for "has_proj_mat" etc. then if user calls set_proj_mat (mtl, mat4) we can see
if that's valid internally */
struct PBR_Mtl {
	GLuint sp;
	GLint P_loc, V_loc, M_loc;
};
typedef struct PBR_Mtl PBR_Mtl;

int main () {
	printf ("PBR experiment - anton gerdelan\n");
	if (!start_gl ()) {
		return 1;
	}
	glEnable (GL_DEPTH_TEST);
  glDepthFunc (GL_LESS);
	glClearColor (0.2, 0.2, 0.2, 1.0);

	GLuint vao = 0, vp_vbo = 0, vt_vbo = 0, vn_vbo = 0;
	int point_count = 0;
	{
		float *points = NULL, *tex_coords = NULL, *normals = NULL;
		printf ("loading mesh...\n");
		if (!load_obj_file ("../common/meshes/armadillo.obj", &points, &tex_coords,
			&normals, &point_count)) {
			fprintf (stderr, "ERROR: loading mesh\n");
			return 1;
		}

		glGenVertexArrays (1, &vao);
		glBindVertexArray (vao);
		glGenBuffers (1, &vp_vbo);
		glBindBuffer (GL_ARRAY_BUFFER, vp_vbo);
		glBufferData (GL_ARRAY_BUFFER, sizeof (float) * 3 * point_count, points,
			GL_STATIC_DRAW);
		// TODO -- float16 for these?
		glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray (0);
		glGenBuffers (1, &vt_vbo);
		glBindBuffer (GL_ARRAY_BUFFER, vt_vbo);
		glBufferData (GL_ARRAY_BUFFER, sizeof (float) * 2 * point_count, tex_coords,
			GL_STATIC_DRAW);
		glVertexAttribPointer (1, 2, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray (1);
		glGenBuffers (1, &vn_vbo);
		glBindBuffer (GL_ARRAY_BUFFER, vn_vbo);
		glBufferData (GL_ARRAY_BUFFER, sizeof (float) * 3 * point_count, normals,
			GL_STATIC_DRAW);
		glVertexAttribPointer (2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray (2);
	}

	mat4 P = perspective (
		67.0,
		(float)g_gl_width / (float)g_gl_height,
		0.1,
		10.0
	);
	mat4 V = look_at (
		vec3_from_3f (0.0, 0.5, 1.0),
		vec3_from_3f (0.0, 0.5, 0.0),
		vec3_from_3f (0.0, 1.0, 0.0)
	);
	mat4 M = identity_mat4 ();

	PBR_Mtl pbr_mtl;
	{
		pbr_mtl.sp = create_programme_from_files ("pbr.vert", "pbr.frag");
		pbr_mtl.P_loc = glGetUniformLocation (pbr_mtl.sp, "P");
		pbr_mtl.V_loc = glGetUniformLocation (pbr_mtl.sp, "V");
		pbr_mtl.M_loc = glGetUniformLocation (pbr_mtl.sp, "M");
		assert (pbr_mtl.P_loc > -1);
		assert (pbr_mtl.V_loc > -1);
		assert (pbr_mtl.M_loc > -1);
		glUseProgram (pbr_mtl.sp);
		glUniformMatrix4fv (pbr_mtl.P_loc, 1, GL_FALSE, P.m);
		glUniformMatrix4fv (pbr_mtl.V_loc, 1, GL_FALSE, V.m);
		glUniformMatrix4fv (pbr_mtl.M_loc, 1, GL_FALSE, M.m);
	}

	printf ("main loop...\n");

	while (!glfwWindowShouldClose (g_window)) {
		if (GLFW_PRESS == glfwGetKey (g_window, GLFW_KEY_ESCAPE)) {
			glfwSetWindowShouldClose (g_window, 1);
		}

		glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport (0, 0, g_gl_width, g_gl_height);

		glBindVertexArray (vao);
		glDrawArrays (GL_TRIANGLES, 0, point_count);

		glfwPollEvents ();
		glfwSwapBuffers (g_window);
	}

	return 0;
}
