//
// WebGL and JavaScript utilities
// Anton Gerdelan 23 Dec 2014
// antongerdelan.net
//

var keys_down = {};

function get_string_from_URL (url) {
	var xmlhttp = new XMLHttpRequest();
	xmlhttp.open ("GET", url, false);
	xmlhttp.send ();
	return xmlhttp.responseText;
}

function load_shaders_from_strings (vs_str, fs_str) {
	var vs = gl.createShader (gl.VERTEX_SHADER);
	var fs = gl.createShader (gl.FRAGMENT_SHADER);
	gl.shaderSource (vs, vs_str);
	gl.shaderSource (fs, fs_str);
	gl.compileShader (vs);
	gl.compileShader (fs);
	var sp = gl.createProgram ();
	gl.attachShader (sp, vs);
	gl.attachShader (sp, fs);
	return sp;
}

function load_shaders_from_files (vs_filename, fs_filename) {
	var vs_str = get_string_from_URL (vs_filename);
	var fs_str = get_string_from_URL (fs_filename);
	var sp = load_shaders_from_strings (vs_str, fs_str);
	return sp;
}

function get_uniform_loc (sp, var_str) {
	var loc = gl.getUniformLocation (sp, var_str);
	if (loc < 0) {
		console.error ("uniform variable not active: " + var_str);
	}
	return loc;
}

function create_texture_from_file (url) {
	console.log ("loading image " + url + "...");
	var texture = gl.createTexture();
	var image = new Image();
	image.onload = function () {
		gl.bindTexture (gl.TEXTURE_2D, texture);
		gl.pixelStorei (gl.UNPACK_FLIP_Y_WEBGL, true);
		gl.texImage2D (gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE,
			image);
		gl.texParameteri (gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
		gl.texParameteri (gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
		gl.texParameteri (gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
		gl.texParameteri (gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
		console.log ("texture loaded from " + url);
	}
	image.src = url;
	return texture;
}

function mult_mat4_mat4 (a, b) {
	var r = [
		0.0, 0.0, 0.0, 0.0,
		0.0, 0.0, 0.0, 0.0,
		0.0, 0.0, 0.0, 0.0,
		0.0, 0.0, 0.0, 0.0
	];
	var r_index = 0;
	for (row = 0; row < 4; row++) {
		for (col = 0; col < 4; col++) {
			var sum = 0.0;
			for (i = 0; i < 4; i++) {
				sum += b[i + row * 4] * a[col + i * 4];
			}
			r[r_index] = sum;
			r_index++;
		}
	}
	return r;
}

document.onkeydown = function (event) {
	keys_down[event.keyCode] = true;
	//console.log ("key down. " + event.keyCode);
}

document.onkeyup = function (event) {
	keys_down[event.keyCode] = false;
	//console.log ("key up. " + event.keyCode);
}
