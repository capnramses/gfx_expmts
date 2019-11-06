//
// Skybox background rendering in WebGL - antongerdelan.net
// Copyright: Anton Gerdelan <antonofnote _at_ gmail _dot_ com> 15th April 2018
//

var canvas;
var cube_shader_vs_id = "skybox.vert";
var cube_shader_fs_id = "skybox.frag";
var cube_texture;
var cube_program;
var cube_program_R_loc;
var cube_vp_vbo;
var cube_nverts;
var mouse_state = {};
mouse_state.clicks = 0;
mouse_state.second_since_click = 0.0;
mouse_state.x = 0;
mouse_state.y = 0;

function create_cube_side_texture_from_file(side, url) {
	console.log("loading image " + url + "...");
	var image = new Image();
	image.onload = function () {
		gl.activeTexture(gl.TEXTURE0);
		gl.bindTexture(gl.TEXTURE_CUBE_MAP, cube_texture);
		//	gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL, true);
		gl.texImage2D(side, 0, gl.RGB, gl.RGB, gl.UNSIGNED_BYTE, image);
		console.log("texture loaded from " + url);
	}
	image.src = url;
}

function main() {
	canvas = document.getElementById("canvas");
	canvas.width = window.innerWidth;
	canvas.height = window.innerHeight;
	
	// TODO(Anton) try webgl1 w/o wrap R when working
	gl = WebGLUtils.setupWebGL(canvas); //canvas.getContext('webgl2'); //

	{ // cube map shaders
		var vs = gl.createShader(gl.VERTEX_SHADER);
		var fs = gl.createShader(gl.FRAGMENT_SHADER);
		var vs_el = document.getElementById(cube_shader_vs_id);
		var fs_el = document.getElementById(cube_shader_fs_id);
		var vs_str = vs_el.innerHTML;
		//console.log(vs_str);
		var fs_str = fs_el.innerHTML;
		gl.shaderSource(vs, vs_str);
		gl.shaderSource(fs, fs_str);
		gl.compileShader(vs);
		gl.compileShader(fs);
		cube_program = gl.createProgram();
		gl.attachShader(cube_program, vs);
		gl.attachShader(cube_program, fs);
		gl.linkProgram(cube_program);
		cube_program_R_loc = gl.getUniformLocation(cube_program, "R");
	}

	{ // cube map texture
		cube_texture = gl.createTexture();

		create_cube_side_texture_from_file(gl.TEXTURE_CUBE_MAP_NEGATIVE_Z, "images/kegs_back.png");
		create_cube_side_texture_from_file(gl.TEXTURE_CUBE_MAP_POSITIVE_Z, "images/kegs_forward.png");
		create_cube_side_texture_from_file(gl.TEXTURE_CUBE_MAP_NEGATIVE_Y, "images/kegs_down.png");
		create_cube_side_texture_from_file(gl.TEXTURE_CUBE_MAP_POSITIVE_Y, "images/kegs_up.png");
		create_cube_side_texture_from_file(gl.TEXTURE_CUBE_MAP_NEGATIVE_X, "images/kegs_left.png");
		create_cube_side_texture_from_file(gl.TEXTURE_CUBE_MAP_POSITIVE_X, "images/kegs_right.png");

		gl.activeTexture(gl.TEXTURE0);
		gl.bindTexture(gl.TEXTURE_CUBE_MAP, cube_texture);
		// needs webgl2 gl.texParameteri(gl.TEXTURE_CUBE_MAP, gl.TEXTURE_WRAP_R, gl.CLAMP_TO_EDGE);
		gl.texParameteri(gl.TEXTURE_CUBE_MAP, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
		gl.texParameteri(gl.TEXTURE_CUBE_MAP, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
		gl.texParameteri(gl.TEXTURE_CUBE_MAP, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
		gl.texParameteri(gl.TEXTURE_CUBE_MAP, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
	}

	{ // geometry
		var cube_points = [
			-1.0, 1.0, -1.0, -1.0, -1.0, -1.0, 1.0, -1.0, -1.0,
			1.0, -1.0, -1.0, 1.0, 1.0, -1.0, -1.0, 1.0, -1.0
		];
		cube_nverts = cube_points.length / 3;
		console.log("cube has npoints: " + cube_nverts)

		cube_vp_vbo = gl.createBuffer();
		gl.bindBuffer(gl.ARRAY_BUFFER, cube_vp_vbo);
		gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(cube_points), gl.STATIC_DRAW);
	}

	{ // input handling
		canvas.onmousemove = function (ev) {
			// recursively get location within parent(s)
			var element = canvas;
			var top = 0;
			var left = 0;
			while (element && element.tagName != 'BODY') {
				top += element.offsetTop;
				left += element.offsetLeft;
				element = element.offsetParent;
			}
			// adjust for scrolling
			left += window.pageXOffset;
			top -= window.pageYOffset;
			mouse_state.x = ev.clientX - left;
			mouse_state.y = (ev.clientY - top);

			mouse_state.y++; // BUGFIX hack
		}
	}

	gl.clearColor(102.0 / 255.0, 2.0 / 255.0, 60.0 / 255.0, 1.0); // Tyrian purple

	previous_millis = performance.now();
	main_loop();
}

function main_loop() {
	canvas.width = window.innerWidth;
	canvas.height = window.innerHeight;

	// update timers
	var current_millis = performance.now();
	var elapsed_millis = current_millis - previous_millis;
	previous_millis = current_millis;
	var elapsed_s = elapsed_millis / 1000.0;

	var x_degrees = -mouse_state.y / canvas.clientHeight * 90.0 + 45.0;
	var y_degrees = -mouse_state.x / canvas.clientWidth * 360.0 + 180.0;
	var yquat = quat_from_axis_deg(y_degrees, 0.0, 1.0, 0.0);
	var xquat = quat_from_axis_deg(x_degrees, 1.0, 0.0, 0.0);
	var xR = quat_to_mat4(xquat);
	var yR = quat_to_mat4(yquat);
	var R = mult_mat4_mat4(yR, xR);

	// TODO(Anton) recalc viewport size basic on window background size
	gl.viewport(0, 0, canvas.clientWidth, canvas.clientHeight);
	gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

	gl.cullFace(gl.BACK);
	gl.frontFace(gl.CCW);
	gl.enable(gl.CULL_FACE);
	gl.enable(gl.DEPTH_TEST);
	gl.disable(gl.BLEND);

	{
		gl.useProgram(cube_program);
		gl.uniformMatrix4fv(cube_program_R_loc, gl.FALSE, new Float32Array(R));
		gl.enableVertexAttribArray(0);
		gl.activeTexture(gl.TEXTURE0);
		gl.bindTexture(gl.TEXTURE_CUBE_MAP, cube_texture);
		gl.bindBuffer(gl.ARRAY_BUFFER, cube_vp_vbo);
		gl.vertexAttribPointer(0, 3, gl.FLOAT, false, 0, 0);

		gl.drawArrays(gl.TRIANGLES, 0, cube_nverts);
	}

	// this function is from webgl-utils
	window.requestAnimFrame(main_loop, canvas);
}

main(); // go!