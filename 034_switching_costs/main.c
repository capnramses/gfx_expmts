// OpenGL with different approaches to redundant state switching
// to be tested by APItrace
// Anton Gerdelan 17 April 2016

#include "gl_utils.h"

void use_sp (GLuint sp) {
	static int in_use = 0;
	if (sp == in_use) { return; }
	glUseProgram (sp); // 47us CPU 0 GPU
	in_use = sp;
	
	int cp = 0;
	glGetIntegerv (GL_CURRENT_PROGRAM, &cp);
	if (sp == cp) { return; }
	glUseProgram (sp);
}

void use_vbo (GLuint vbo) {
	static int in_use = 0;
	if (vbo == in_use) { return; }
	glBindVertexArray (vbo); // 47us CPU 0 GPU
	in_use = vbo;
	
	GLint curr_b = 0;
	glGetIntegerv (GL_ARRAY_BUFFER_BINDING, &curr_b);
	if ((GLint)vbo == curr_b) { return; }
	glBindBuffer (GL_ARRAY_BUFFER, vbo);
}

int main() {
	start_gl ();
	GLuint quad_vao = create_quad_vao ();
	GLuint quad_program = create_quad_program ();
	double prev_s = glfwGetTime ();
	double accum_s = 0.0;
	int accum_f = 0;
	while (!glfwWindowShouldClose (window)) { // drawing loop
		double curr_s = glfwGetTime ();
		double elap_s = curr_s - prev_s;
		prev_s = curr_s;
		accum_s += elap_s;
		if (accum_s > 0.5 && accum_f > 0) {
			double elap_ms = (accum_s * 1000.0) / (double)accum_f;
			double fps = (double)accum_f / accum_s;
			accum_s = 0.0;
			accum_f = 0;
			char tit_str[256];
			sprintf (tit_str, "%.0f fps %.2f ms\n", fps, elap_ms);
			glfwSetWindowTitle (window, tit_str);
		}
		{
			glfwPollEvents ();
			if (GLFW_PRESS == glfwGetKey (window, GLFW_KEY_ESCAPE)) {
				glfwSetWindowShouldClose (window, 1);
			}
		}
		
		// NVIDIA linux driver:
		// no checks - always bind both - 9200fps
		// eliminate 2nd bind+use but always bind first - 9300fps
		// eliminate 2nd bind+use but check first - 9300fps
		// custom tracking of state - 9400fps
		// glGet checks - 9400 fps
		
		glClear (GL_COLOR_BUFFER_BIT); // 72us CPU, 1.8us GPU
		use_sp (quad_program); // 47us CPU 0 GPU
		use_vbo (quad_vao); // 43us CPU 0 GPU
		glDrawArrays (GL_TRIANGLE_STRIP, 0, 4); // 56us CPU, 12us GPU (262,144px drawn)
		use_sp (quad_program); // 47us CPU 0 GPU
		use_vbo (quad_vao); // 43us CPU 0 GPU
		glDrawArrays (GL_TRIANGLE_STRIP, 0, 4); // 56us CPU, 12us GPU (262,144px drawn)
		glfwSwapBuffers (window);
		accum_f++;
	}
	stop_gl (); // stop glfw, close window
	return 0;
}
