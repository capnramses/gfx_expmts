//
// Wavefront .obj viewer for WebGL
// Anton Gerdelan 23 Dec 2014
// antongerdelan.net
//
// TODO parse_obj_into_vbos

var canvas;
var obj_fn = "meshes/suzanne.obj"
var tex;
var sp;
var M_loc, V_loc, P_loc;
var M;
var deg = 0.0;
var g_axis_vp_vbo, g_axis_vc_vbo, g_axes_local_vp_vbo, g_axes_local_vc_vbo;
var g_axis_P_loc, g_axis_V_loc;
var q;
var g_axis_v = [0.0, 1.0, 0.0];
var auto_rotate = true;
var g_button_pressed = true;
var g_axis_changed = false;
var previous_millis;
var mesh;

var g_time_step_size_s = 1.0 / 50.0; // smallest fixed-step update in seconds

// when you're typing the axis of rotation should visibly update
function axis_cb() {
	g_axis_changed = true;
}

// when button clicked, flag to do a rotation
function update_cb() {
	g_button_pressed = true;
}

// switch auto-rotate mode on checkbox click
function checkbox_cb() {
	auto_rotate = document.getElementById ("auto_rotate").checked;
}

function main () {
	canvas = document.getElementById ("canvas");
	gl = WebGLUtils.setupWebGL (canvas);
	//tex = create_texture_from_file (img);
	mesh = parse_obj_into_vbos (obj_fn);
	{ // monkey shaders
		var el = document.getElementById ("basic.vert");
		var vs_str = el.innerHTML;
		el = document.getElementById ("basic.frag");
		var fs_str = el.innerHTML;
		sp = load_shaders_from_strings (vs_str, fs_str);
		gl.bindAttribLocation (sp, 0, "vp");
		gl.bindAttribLocation (sp, 1, "vt");
		gl.bindAttribLocation (sp, 2, "vn");
		gl.linkProgram (sp);
		M_loc = get_uniform_loc (sp, "M");
		P_loc = get_uniform_loc (sp, "P");
		V_loc = get_uniform_loc (sp, "V");
	}
	{ // axis shaders
		var el = document.getElementById ("axis.vert");
		var vs_str = el.innerHTML;
		el = document.getElementById ("axis.frag");
		var fs_str = el.innerHTML;
		g_axis_sp = load_shaders_from_strings (vs_str, fs_str);
		gl.bindAttribLocation (g_axis_sp, 0, "vp");
		gl.bindAttribLocation (g_axis_sp, 1, "vc");
		gl.linkProgram (g_axis_sp);
		g_axis_P_loc = get_uniform_loc (g_axis_sp, "P");
		g_axis_V_loc = get_uniform_loc (g_axis_sp, "V");
	}
	{ // axis for rotation
		g_axis_vp_vbo = gl.createBuffer ();
		var vp = [
			0.0, 3.0, 0.0,
			0.0, -3.0, 0.0
		];
		gl.bindBuffer (gl.ARRAY_BUFFER, g_axis_vp_vbo);
		gl.bufferData (gl.ARRAY_BUFFER, new Float32Array (vp), gl.STATIC_DRAW);
		g_axis_vc_vbo = gl.createBuffer ();
		var vc = [
			0.5, 0.5, 0.5,
			0.0, 0.0, 0.0
		];
		gl.bindBuffer (gl.ARRAY_BUFFER, g_axis_vc_vbo);
		gl.bufferData (gl.ARRAY_BUFFER, new Float32Array (vc), gl.STATIC_DRAW);
	}
	{ // axes for local fwd/rgt/up
		g_axes_local_vp_vbo = gl.createBuffer ();
		var vp = [
			0.0, 2.0, 0.0,
			0.0, 0.0, 0.0,
			2.0, 0.0, 0.0,
			0.0, 0.0, 0.0,
			0.0, 0.0, 2.0,
			0.0, 0.0, 0.0
		];
		gl.bindBuffer (gl.ARRAY_BUFFER, g_axes_local_vp_vbo);
		gl.bufferData (gl.ARRAY_BUFFER, new Float32Array (vp), gl.STATIC_DRAW);
		g_axes_local_vc_vbo = gl.createBuffer ();
		var vc = [
			0.0, 1.0, 0.0,
			0.0, 1.0, 0.0,
			1.0, 0.0, 0.0,
			1.0, 0.0, 0.0,
			0.0, 0.0, 1.0,
			0.0, 0.0, 1.0
		];
		gl.bindBuffer (gl.ARRAY_BUFFER, g_axes_local_vc_vbo);
		gl.bufferData (gl.ARRAY_BUFFER, new Float32Array (vc), gl.STATIC_DRAW);
	}
	
	q = quat_from_axis_deg (0.0, 0.0, 1.0, 0.0);
	M = identity_mat4 ();
	var V = translate_mat4 (identity_mat4 (), [0.0, 0.0, -5.0]);
	var P = perspective (67.0, 1.0, 0.1, 100.0);
	gl.useProgram (sp);
	gl.uniformMatrix4fv (M_loc, gl.FALSE, new Float32Array (M));
	gl.uniformMatrix4fv (V_loc, gl.FALSE, new Float32Array (V));
	gl.uniformMatrix4fv (P_loc, gl.FALSE, new Float32Array (P));
	gl.useProgram (g_axis_sp);
	gl.uniformMatrix4fv (g_axis_V_loc, gl.FALSE, new Float32Array (V));
	gl.uniformMatrix4fv (g_axis_P_loc, gl.FALSE, new Float32Array (P));
	gl.cullFace (gl.BACK);
	gl.frontFace (gl.CCW);
	gl.enable (gl.CULL_FACE);
	gl.enable (gl.DEPTH_TEST);
	gl.disable (gl.BLEND);
	//gl.clearColor (0.0, 0.0, 0.0, 1.0);
	gl.clearColor (1.0, 1.0, 1.0, 1.0);
	gl.viewport (0, 0, canvas.clientWidth, canvas.clientHeight);
	previous_millis = performance.now ();
	main_loop ();
}

function update_axis () {
	var x = parseFloat (document.getElementById ("quat_x_axis_input").value);
	if (isNaN (x)) {
		return;
	}
	var y = parseFloat (document.getElementById ("quat_y_axis_input").value);
	if (isNaN (y)) {
		return;
	}
	var z = parseFloat (document.getElementById ("quat_z_axis_input").value);
	if (isNaN (z)) {
		return;
	}
	g_axis_v = normalise_vec3 ([x, y, z]);
	var vp = [
		g_axis_v[0] * 3.0, g_axis_v[1] * 3.0, g_axis_v[2] * 3.0,
		-g_axis_v[0] * 3.0, -g_axis_v[1] * 3.0, -g_axis_v[2] * 3.0
	];
	gl.bindBuffer (gl.ARRAY_BUFFER, g_axis_vp_vbo);
	gl.bufferData (gl.ARRAY_BUFFER, new Float32Array (vp), gl.STATIC_DRAW);
	var normd_el = document.getElementById ("normd");
	normd_el.innerHTML = "normalise(axis) = [" + g_axis_v[0].toFixed(2) +
		", " + g_axis_v[1].toFixed(2) + ", " + g_axis_v[2].toFixed(2) +
		"]<br />";
}

function main_loop () {
	// update timers
  var current_millis = performance.now ();
  var elapsed_millis = current_millis - previous_millis;
  previous_millis = current_millis;
  var elapsed_s = elapsed_millis / 1000.0;

	draw ();

  if (g_axis_changed) {
  	update_axis ();
  	g_axis_changed = false;
  }

	if (g_button_pressed || auto_rotate) {
		var cdeg = 0.0;
		if (g_button_pressed) {
			if (!auto_rotate) {
				var quat_angle_el = document.getElementById ("quat_angle");
				cdeg = parseFloat (quat_angle_el.value);
			}
			g_button_pressed = false;
		}
	  if (auto_rotate) {
			cdeg = elapsed_s * 100.0;
		}
		if (!isNaN (cdeg)) {
			deg = cdeg;
		}

		q = quat_from_axis_deg (deg, g_axis_v[0], g_axis_v[1], g_axis_v[2]);

		var versor_el = document.getElementById ("versor");
		versor_el.innerHTML = "versor = [<br />" +
		"&nbsp;&nbsp;cos (angle),<br />" +
		"&nbsp;&nbsp;x * sin (0.5 * angle),<br />" +
		"&nbsp;&nbsp;y * sin (0.5 * angle),<br />" +
		"&nbsp;&nbsp;z * sin (0.5 * angle)<br />" +
		"]<br />" +
		"= [" + q[0].toFixed(2) + ", " + q[1].toFixed(2) + ", " +
		q[2].toFixed(2) + ", " + q[3].toFixed(2) + "]";

		var R = quat_to_mat4 (q);
		M = mult_mat4_mat4 (R, M); // NOTE: This is M * R, not R * M !!!

		{
			var rgt = mult_mat4_vec4 (M, [-1.0, 0.0, 0.0, 0.0]);
			var up = mult_mat4_vec4 (M, [0.0, 1.0, 0.0, 0.0]);
			var fwd = cross_vec3 (up, rgt);

			var local_axes_el = document.getElementById ("local_axes");
			local_axes_el.innerHTML = "rgt = R * [-1.0, 0.0, 0.0, 0.0]<br />" +
				"= [" + rgt[0].toFixed(2) + ", " + rgt[1].toFixed(2) + ", " +
				rgt[2].toFixed(2) + "]<br />" +
				"up = R * [0.0, 1.0, 0.0, 0.0]<br />" +
				"= [" + up[0].toFixed(2) + ", " + up[1].toFixed(2) + ", " +
				up[2].toFixed(2) + "]<br />" +
				"fwd = up X rgt<br />" +
				"= [" + fwd[0].toFixed(2) + ", " + fwd[1].toFixed(2) + ", " +
				fwd[2].toFixed(2) + "]<br />";

			var vp = [
				up[0] * 2.0, up[1] * 2.0, up[2] * 2.0,
				0.0, 0.0, 0.0,
				rgt[0] * 2.0, rgt[1] * 2.0, rgt[2] * 2.0,
				0.0, 0.0, 0.0,
				fwd[0] * 2.0, fwd[1] * 2.0, fwd[2] * 2.0,
				0.0, 0.0, 0.0
			];
			gl.bindBuffer (gl.ARRAY_BUFFER, g_axes_local_vp_vbo);
			gl.bufferData (gl.ARRAY_BUFFER, new Float32Array (vp), gl.STATIC_DRAW);
		}

		var str = "<br />" + "&nbsp;&nbsp;| " + R[0].toFixed (2) + " " + R[4].toFixed (2) + " " +
			R[8].toFixed (2) + " " + R[12].toFixed (2) + " |<br />" +
			"= | " + R[1].toFixed (2) + " " + R[5].toFixed (2) + " " +
			R[9].toFixed (2) + " " + R[13].toFixed (2) + " |<br />" +
			"&nbsp;&nbsp;| " + R[2].toFixed (2) + " " + R[6].toFixed (2) + " " +
			R[10].toFixed (2) + " " + R[14].toFixed (2) + " |<br />" +
			"&nbsp;&nbsp;| " + R[3].toFixed (2) + " " + R[7].toFixed (2) + " " +
			R[11].toFixed (2) + " " + R[15].toFixed (2) + " |";

		var qtoM_el = document.getElementById ("qtoM");
		qtoM_el.innerHTML = "R = quaternion_to_matrix(versor)<br />" + str;
	}

	//print_mat4 (M);
	

	// this function is from webgl-utils
	window.requestAnimFrame (main_loop, canvas);
}

function draw () {
	gl.clear (gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

	if (mesh.is_loaded) { // mesh
		gl.useProgram (sp);
		gl.uniformMatrix4fv (M_loc, gl.FALSE, new Float32Array (M));
		//gl.activeTexture (gl.TEXTURE0);
		//gl.bindTexture (gl.TEXTURE_2D, tex);
		gl.bindBuffer (gl.ARRAY_BUFFER, mesh.vbo_vp);
		gl.vertexAttribPointer (0, 3, gl.FLOAT, false, 0, 0);
		gl.bindBuffer (gl.ARRAY_BUFFER, mesh.vbo_vt);
		gl.vertexAttribPointer (1, 2, gl.FLOAT, false, 0, 0);
		gl.bindBuffer (gl.ARRAY_BUFFER, mesh.vbo_vn);
		gl.vertexAttribPointer (2, 3, gl.FLOAT, false, 0, 0);
		gl.enableVertexAttribArray (0);
		gl.enableVertexAttribArray (1);
		gl.enableVertexAttribArray (2);
		gl.drawArrays (gl.TRIANGLES, 0, mesh.pc);
		gl.disableVertexAttribArray (0);
		gl.disableVertexAttribArray (1);
		gl.disableVertexAttribArray (2);
	}
	{ // axes/lines
		gl.useProgram (g_axis_sp);
		gl.enableVertexAttribArray (0);
		gl.enableVertexAttribArray (1);
		// local axes
		gl.bindBuffer (gl.ARRAY_BUFFER, g_axes_local_vp_vbo);
		gl.vertexAttribPointer (0, 3, gl.FLOAT, false, 0, 0);
		gl.bindBuffer (gl.ARRAY_BUFFER, g_axes_local_vc_vbo);
		gl.vertexAttribPointer (1, 3, gl.FLOAT, false, 0, 0);
		gl.drawArrays (gl.LINES, 0, 6);
		// rotation
		gl.bindBuffer (gl.ARRAY_BUFFER, g_axis_vp_vbo);
		gl.vertexAttribPointer (0, 3, gl.FLOAT, false, 0, 0);
		gl.bindBuffer (gl.ARRAY_BUFFER, g_axis_vc_vbo);
		gl.vertexAttribPointer (1, 3, gl.FLOAT, false, 0, 0);
		gl.drawArrays (gl.LINES, 0, 2);
		//
		gl.disableVertexAttribArray (0);
		gl.disableVertexAttribArray (1);
	}
}
