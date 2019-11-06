//
// anton gerdelan 22 jan 2016 <gerdela@scss.tcd.ie>
// trinity college dublin, ireland
//

//
// TODO:
// start with a simpler demo and build up to the bigger stuff

#include "apg_maths.h"
#include "obj_parser.h"
#include "apg_gl.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#define MESH_FILE "../common/mesh/suzanne.obj"
APG_Mesh cube_mesh;
GLuint shader_programme, dshader_programme;
GLint sp_P_loc = -1, sp_V_loc = - 1, sp_M_loc = -1;
GLint dsp_P_loc = -1, dsp_V_loc = -1, dsp_M_loc = -1, sp_c_loc = -1;
mat4 P, V, g_caster_V[6], g_caster_P;
vec3 cam_pos;
GLuint g_fb, g_fb_tex;

static bool init_fb () {
	int width = 1024, height = 1024;
	printf ("cube face w %i h %i\n", width, height);

	glGenFramebuffers (1, &g_fb);
	glBindFramebuffer (GL_FRAMEBUFFER, g_fb);

	glGenTextures (1, &g_fb_tex);
	glBindTexture (GL_TEXTURE_CUBE_MAP, g_fb_tex);
	glTexParameteri (GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri (GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri (GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_REPEAT);
	glTexParameteri (GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri (GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_REPEAT);
	for (int camd = 0; camd < 6; camd++) {
		glTexImage2D (GL_TEXTURE_CUBE_MAP_POSITIVE_X + camd, 0, GL_RGBA,
			width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glFramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			GL_TEXTURE_CUBE_MAP_POSITIVE_X + camd, g_fb_tex, 0);
	}
	GLuint depthtex;
	glGenTextures (1, &depthtex);
	glBindTexture (GL_TEXTURE_CUBE_MAP, depthtex);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	for (int camd = 0; camd < 6; camd++) {
		glTexImage2D (GL_TEXTURE_CUBE_MAP_POSITIVE_X + camd, 0, GL_DEPTH_COMPONENT,
			width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
		glFramebufferTexture2D (GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
			GL_TEXTURE_CUBE_MAP_POSITIVE_X + camd, depthtex, 0);
	}
	
	/* validate the framebuffer - an 'incomplete' error tells us if an invalid
	image format is attached or if the glDrawBuffers information is invalid */
	GLenum status = glCheckFramebufferStatus (GL_FRAMEBUFFER);
	if (GL_FRAMEBUFFER_COMPLETE != status) {
		fprintf (stderr, "ERROR: incomplete framebuffer\n");
		if (GL_FRAMEBUFFER_UNDEFINED == status) {
			fprintf (stderr, "GL_FRAMEBUFFER_UNDEFINED\n");
		} else if (GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT == status) {
			fprintf (stderr, "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT\n");
		} else if (GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT == status) {
			fprintf (stderr, "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT\n");
		} else if (GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER == status) {
			fprintf (stderr, "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER\n");
		} else if (GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER== status) {
			fprintf (stderr, "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER\n");
		} else if (GL_FRAMEBUFFER_UNSUPPORTED == status) {
			fprintf (stderr, "GL_FRAMEBUFFER_UNSUPPORTED\n");
		} else if (GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE == status) {
			fprintf (stderr, "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE\n");
		} else if (GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS == status) {
			fprintf (stderr, "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS\n");
		} else {
			fprintf (stderr, "unspecified error\n");
		}
		return false;
	}
	
	/* re-bind the default framebuffer as a safe precaution */
	glBindFramebuffer (GL_FRAMEBUFFER, 0);
	return true;
}

static void init () {
	assert (start_gl ());
	assert (init_fb ());
	{ // cube
		glGenVertexArrays (1, &cube_mesh.vao);
		glBindVertexArray (cube_mesh.vao);
		glGenBuffers (1, &cube_mesh.vbo_vps);
		glBindBuffer (GL_ARRAY_BUFFER, cube_mesh.vbo_vps);
		cube_mesh.pc = APG_CUBE_POINT_COUNT;
		glBufferData (GL_ARRAY_BUFFER, APG_CUBE_POINTS_SZ, apg_cube_points,
			GL_STATIC_DRAW);
		glEnableVertexAttribArray (0);
		glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		glGenBuffers (1, &cube_mesh.vbo_indexed);
		glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, cube_mesh.vbo_indexed);
		glBufferData (GL_ELEMENT_ARRAY_BUFFER, APG_CUBE_INDICES_SZ,
			apg_cube_ccw_tri_indices, GL_STATIC_DRAW);
	}
	{ // load shaders
		const char* dvertex_shader =
			"#version 430\n"
			"in vec3 vp;"
			"uniform mat4 P, V, M;"
			"out vec4 p;"
			"void main () {"
			"  gl_Position = P * V * M * vec4 (vp, 1.0);"
			"  p = V * M * vec4 (vp * vp.z, 1.0);"
			"}";
		const char* dfragment_shader =
			"#version 430\n"
			"in vec4 p;"
			"out vec4 fc;"
			"void main () {"
			"  fc = vec4 (p);" // hack to make sure SOMETHING is writ
			"}";
		GLuint dvs = glCreateShader (GL_VERTEX_SHADER);
		glShaderSource (dvs, 1, &dvertex_shader, NULL);
		glCompileShader (dvs);
		GLuint dfs = glCreateShader (GL_FRAGMENT_SHADER);
		glShaderSource (dfs, 1, &dfragment_shader, NULL);
		glCompileShader (dfs);
		dshader_programme = glCreateProgram ();
		glAttachShader (dshader_programme, dfs);
		glAttachShader (dshader_programme, dvs);
		glLinkProgram (dshader_programme);
		dsp_P_loc = glGetUniformLocation (dshader_programme, "P");
		dsp_V_loc = glGetUniformLocation (dshader_programme, "V");
		dsp_M_loc = glGetUniformLocation (dshader_programme, "M");
		assert (dsp_P_loc > -1);
		assert (dsp_V_loc > -1);
		assert (dsp_M_loc > -1);
		const char* vertex_shader =
			"#version 430\n"
			"in vec3 vp;"
			"out vec3 p;"
			"uniform mat4 P, V, M;"
			"void main () {"
			"  gl_Position = P * V * M * vec4 (vp, 1.0);"
			"  p = vp;"
			"}";
		const char* fragment_shader =
			"#version 430\n"
			"in vec3 p;"
			"uniform vec3 c;"
		//	"uniform samplerCube tex;"
			"out vec4 fc;"
			"void main () {"
			"  fc = vec4 (c, 1.0);"
		//	"  vec4 texel = texture (tex, p);"
		//	"  fc = vec4 (texel.rgb, 1.0);"
			"}";
		GLuint vs = glCreateShader (GL_VERTEX_SHADER);
		glShaderSource (vs, 1, &vertex_shader, NULL);
		glCompileShader (vs);
		GLuint fs = glCreateShader (GL_FRAGMENT_SHADER);
		glShaderSource (fs, 1, &fragment_shader, NULL);
		glCompileShader (fs);
		shader_programme = glCreateProgram ();
		glAttachShader (shader_programme, fs);
		glAttachShader (shader_programme, vs);
		glLinkProgram (shader_programme);
		sp_P_loc = glGetUniformLocation (shader_programme, "P");
		sp_V_loc = glGetUniformLocation (shader_programme, "V");
		sp_M_loc = glGetUniformLocation (shader_programme, "M");
		sp_c_loc = glGetUniformLocation (shader_programme, "c");
	}
	{ // camera
		cam_pos = vec3_from_3f (0.0f, 0.0f, 60.0f);
		float a = (float)g_gl.fb_width / (float)g_gl.fb_height;
		P = perspective (67.0f, a, 0.1f, 1000.0f);
		V = look_at (cam_pos,
			vec3_from_3f (0.0f, 0.0f, -1.0f),
			vec3_from_3f (0.0f, 1.0f, 0.0f));
	}
	{ // shadow caster
		vec3 light_pos = vec3_from_3f (0,0,0);
		g_caster_V[0] = look_at (light_pos, add_vec3_vec3 (light_pos, vec3_from_3f (1,0,0)), vec3_from_3f (0,1,0));
		g_caster_V[1] = look_at (light_pos, add_vec3_vec3 (light_pos, vec3_from_3f (-1,0,0)), vec3_from_3f (0,1,0));
		g_caster_V[2] = look_at (light_pos, add_vec3_vec3 (light_pos, vec3_from_3f (0,1,0)), vec3_from_3f (0,0,1));
		g_caster_V[3] = look_at (light_pos, add_vec3_vec3 (light_pos, vec3_from_3f (0,-1,0)), vec3_from_3f (0,0,-1));
		g_caster_V[4] = look_at (light_pos, add_vec3_vec3 (light_pos, vec3_from_3f (0,0,1)), vec3_from_3f (0,1,0));
		g_caster_V[5] = look_at (light_pos, add_vec3_vec3 (light_pos, vec3_from_3f (0,0,-1)), vec3_from_3f (0,1,0));
		// create a projection matrix for the shadow caster
		float near = 0.1f;
		float far = 100.0f;
		float fov = 90.0f; // TODO 45??
		float aspect = 1.0f;
		g_caster_P = perspective (fov, aspect, near, far);
	}
}

int main () {
	init ();
	{ // running
		int f_accum = 0;
		double s_accum = 0.0;
		double prev = glfwGetTime ();
		while (!glfwWindowShouldClose (g_gl.win)) {
			// timers and counters
			double curr = glfwGetTime ();
			double elapsed = curr - prev;
			prev = curr;
			s_accum += elapsed;
			if (s_accum > 0.5 && f_accum > 0) {
				double ms_per_frame = (s_accum / (double)f_accum) * 1000.0;
				if (ms_per_frame > 0.0) {
					double fps = 1000.0 / ms_per_frame;
					char tmp[1024];
					sprintf (tmp, "CPU: %.2fms %.2ffps\n", ms_per_frame, fps);
					glfwSetWindowTitle (g_gl.win, tmp);
				}
				s_accum = 0.0;
				f_accum = 0;
			}
			{ // user input
				glfwPollEvents ();
				if (glfwGetKey (g_gl.win, GLFW_KEY_ESCAPE)) {
					break;
				}
				if (glfwGetKey (g_gl.win, GLFW_KEY_W)) {
					cam_pos.v[2] -= 20.0f * elapsed;
					V = look_at (cam_pos,
						add_vec3_vec3 (cam_pos, vec3_from_3f (0.0f, 0.0f, -1.0f)),
						vec3_from_3f (0.0f, 1.0f, 0.0f)
					);
					print_vec3 (cam_pos);
				}
				if (glfwGetKey (g_gl.win, GLFW_KEY_S)) {
					cam_pos.v[2] += 20.0f * elapsed;
					V = look_at (cam_pos,
						add_vec3_vec3 (cam_pos, vec3_from_3f (0.0f, 0.0f, -1.0f)),
						vec3_from_3f (0.0f, 1.0f, 0.0f)
					);
					print_vec3 (cam_pos);
				}
				if (glfwGetKey (g_gl.win, GLFW_KEY_A)) {
					cam_pos.v[0] -= 20.0f * elapsed;
					V = look_at (cam_pos,
						add_vec3_vec3 (cam_pos, vec3_from_3f (0.0f, 0.0f, -1.0f)),
						vec3_from_3f (0.0f, 1.0f, 0.0f)
					);
					print_vec3 (cam_pos);
				}
				if (glfwGetKey (g_gl.win, GLFW_KEY_D)) {
					cam_pos.v[0] += 20.0f * elapsed;
					V = look_at (cam_pos,
						add_vec3_vec3 (cam_pos, vec3_from_3f (0.0f, 0.0f, -1.0f)),
						vec3_from_3f (0.0f, 1.0f, 0.0f)
					);
					print_vec3 (cam_pos);
				}
				if (glfwGetKey (g_gl.win, GLFW_KEY_Q)) {
					cam_pos.v[1] -= 20.0f * elapsed;
					V = look_at (cam_pos,
						add_vec3_vec3 (cam_pos, vec3_from_3f (0.0f, 0.0f, -1.0f)),
						vec3_from_3f (0.0f, 1.0f, 0.0f)
					);
					print_vec3 (cam_pos);
				}
				if (glfwGetKey (g_gl.win, GLFW_KEY_E)) {
					cam_pos.v[1] += 20.0f * elapsed;
					V = look_at (cam_pos,
						add_vec3_vec3 (cam_pos, vec3_from_3f (0.0f, 0.0f, -1.0f)),
						vec3_from_3f (0.0f, 1.0f, 0.0f)
					);
					print_vec3 (cam_pos);
				}
			}
			//glDepthFunc (GL_LESS);
			// NOTE: HAS TO GO _BEFORE_ glClear
			//glDepthMask (GL_TRUE);

			

			// TODO shadow passes here
			// TODO camera VP
			// TODO assemble cube




///////////////////////////?RENDER TO CUBE MAPS HERE ?///////////////////////

/// HACK always write
//glDisable (GL_CULL_FACE);
				//glDisable (GL_DEPTH_TEST);
			//	glDepthMask (GL_FALSE);
			glDepthFunc (GL_LEQUAL);

			glBindFramebuffer (GL_FRAMEBUFFER, g_fb);
			glViewport (0, 0, g_gl.fb_width, g_gl.fb_height);
glClearColor (0.2,0.2,0.2,1.0);
					glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			{ // depth render pass
				glUseProgram (dshader_programme);
				glBindVertexArray (cube_mesh.vao);
				glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, cube_mesh.vbo_indexed);
			
				for (int camd = 0; camd < 6; camd++) {
				
					// bind for writing
					glBindTexture (GL_TEXTURE_CUBE_MAP, g_fb_tex);
					glFramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
						GL_TEXTURE_CUBE_MAP_POSITIVE_X + camd, g_fb_tex, 0);
					
					glUniformMatrix4fv (dsp_P_loc, 1, GL_FALSE, g_caster_P.m);
					glUniformMatrix4fv (dsp_V_loc, 1, GL_FALSE, g_caster_V[camd].m);

					{ // inside-out cube
						mat4 M = scale_mat4 (vec3_from_3f (30.0f, 30.0f, 30.0f));
						glUniformMatrix4fv (dsp_M_loc, 1, GL_FALSE, M.m);
						glCullFace (GL_FRONT);
						glDrawElements (GL_TRIANGLES, cube_mesh.pc, GL_UNSIGNED_INT, 0);
						glCullFace (GL_BACK);
					}
					{ // middle cubes
						glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, cube_mesh.vbo_indexed);
						for (int z = 0; z < 5; z++) {
							for (int y = 0; y < 5; y++) {
								for (int x = 0; x < 5; x++) {
									mat4 M = translate_mat4 (vec3_from_3f (
										-20.0f + 10.0f * (float)x,
										-20.0f + 10.0f * (float)y,
										-20.0f + 10.0f * (float)z
									));
									glUniformMatrix4fv (dsp_M_loc, 1, GL_FALSE, M.m);
									glDrawElements (GL_TRIANGLES, cube_mesh.pc, GL_UNSIGNED_INT,
										0);
								}
							}
						}
					}
					// ANTON: note: i guess i need this?
					//glCopyTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + camd, 0, 0, 0, 0, 0, g_gl.fb_width, g_gl.fb_height);
				} // endfor
				//glDepthFunc (GL_LEQUAL); // because self is gonna be equal duh!
				//glDepthMask (GL_FALSE); // disable depth writing - already done
			}

// end the HACK that makes sure the depth buffer isnt preventing writes
glEnable (GL_CULL_FACE);
				glEnable (GL_DEPTH_TEST);
				glDepthMask (GL_TRUE);
glDepthFunc (GL_LESS);
///////////////////////////?RENDER NORMALLY HERE ?///////////////////////





			
			
			
			
			
			
			
			
			glBindFramebuffer (GL_FRAMEBUFFER, 0);
			glViewport (0, 0, g_gl.fb_width, g_gl.fb_height);
			glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			
			// TODO sample shadow stuff in here
			
			{
				glUseProgram (shader_programme);
				glBindVertexArray (cube_mesh.vao);
				glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, cube_mesh.vbo_indexed);
				glUniformMatrix4fv (sp_P_loc, 1, GL_FALSE, P.m);
				glUniformMatrix4fv (sp_V_loc, 1, GL_FALSE, V.m);
				glActiveTexture (GL_TEXTURE0);
				glBindTexture (GL_TEXTURE_CUBE_MAP, g_fb_tex);
				// testing mode
				if (glfwGetKey (g_gl.win, GLFW_KEY_SPACE)) {
					glUniformMatrix4fv (sp_P_loc, 1, GL_FALSE, g_caster_P.m);
					glUniformMatrix4fv (sp_V_loc, 1, GL_FALSE, g_caster_V[0].m);
				}
				{ // inside-out cube
					mat4 M = scale_mat4 (vec3_from_3f (30.0f, 30.0f, 30.0f));
					glUniformMatrix4fv (sp_M_loc, 1, GL_FALSE, M.m);
					glUniform3f (sp_c_loc, 0.2f, 0.8f, 0.2f);
					glCullFace (GL_FRONT);
					glDrawElements (GL_TRIANGLES, cube_mesh.pc, GL_UNSIGNED_INT, 0);
					glCullFace (GL_BACK);
				}
				{ // middle cubes
					glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, cube_mesh.vbo_indexed);
					for (int z = 0; z < 5; z++) {
						for (int y = 0; y < 5; y++) {
							for (int x = 0; x < 5; x++) {
								mat4 M = translate_mat4 (vec3_from_3f (
									-20.0f + 10.0f * (float)x,
									-20.0f + 10.0f * (float)y,
									-20.0f + 10.0f * (float)z
								));
								glUniformMatrix4fv (sp_M_loc, 1, GL_FALSE, M.m);
								glUniform3f (sp_c_loc, 0.2f, 0.2f, 0.8f);
								glDrawElements (GL_TRIANGLES, cube_mesh.pc, GL_UNSIGNED_INT,
									0);
							}
						}
					}
				}
			}
			
			{ // screen-space stuff
				glDisable (GL_CULL_FACE);
				glDisable (GL_DEPTH_TEST);
				glDepthMask (GL_FALSE);
				glUseProgram (g_ss_quad_shader.sp);
				glBindVertexArray (ss_quad_tristrip_vao);
				/*{ // whole screen
					glUniform2f (g_ss_quad_shader.sca_loc, 1.0f, 1.0f);
					glUniform2f (g_ss_quad_shader.pos_loc, 0.0f, 0.0f);
					glActiveTexture (GL_TEXTURE0);
					glBindTexture (GL_TEXTURE_2D, g_fb_tex);
					glDrawArrays (GL_TRIANGLE_STRIP, 0, 4);
				}*/
				glActiveTexture (GL_TEXTURE0);
				glBindTexture (GL_TEXTURE_CUBE_MAP, g_fb_tex);
				{ // bottom
					glUniform2f (g_ss_quad_shader.sca_loc, 0.15f, 0.15f);
					glUniform2f (g_ss_quad_shader.pos_loc, -0.85f, 0.85f);
					glDrawArrays (GL_TRIANGLE_STRIP, 0, 4);
				}
				{ // front
					glUniform2f (g_ss_quad_shader.sca_loc, 0.15f, 0.15f);
					glUniform2f (g_ss_quad_shader.pos_loc, -0.55f, 0.85f);
					glDrawArrays (GL_TRIANGLE_STRIP, 0, 4);
				}
				{ // top
					glUniform2f (g_ss_quad_shader.sca_loc, 0.15f, 0.15f);
					glUniform2f (g_ss_quad_shader.pos_loc, -0.25f, 0.85f);
					glDrawArrays (GL_TRIANGLE_STRIP, 0, 4);
				}
				{ // left
					glUniform2f (g_ss_quad_shader.sca_loc, 0.15f, 0.15f);
					glUniform2f (g_ss_quad_shader.pos_loc, -0.85f, 0.55f);
					glDrawArrays (GL_TRIANGLE_STRIP, 0, 4);
				}
				{ // back
					glUniform2f (g_ss_quad_shader.sca_loc, 0.15f, 0.15f);
					glUniform2f (g_ss_quad_shader.pos_loc, -0.55f, 0.55f);
					glDrawArrays (GL_TRIANGLE_STRIP, 0, 4);
				}
				{ // right
					glUniform2f (g_ss_quad_shader.sca_loc, 0.15f, 0.15f);
					glUniform2f (g_ss_quad_shader.pos_loc, -0.25f, 0.55f);
					glDrawArrays (GL_TRIANGLE_STRIP, 0, 4);
				}
				glDepthMask (GL_TRUE);
				glEnable (GL_DEPTH_TEST);
				glEnable (GL_CULL_FACE);
			}
			glfwSwapBuffers (g_gl.win);
			f_accum ++;
		} // endwhile
	} // endblock
	stop_gl ();
	return 0;
}

