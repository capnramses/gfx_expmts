#version 410 core

in vec3 a_vp;
in vec2 a_vt;
in vec3 a_vn;
in vec3 a_vc;
in vec3 a_vtan; // TODO(Anton) probably should be vec4 with -1 w option

uniform mat4 u_P, u_V, u_M;

out vec3 v_p_wor;
out vec2 v_st;
out vec3 v_n_wor;
out vec3 v_tan;
out mat3 TBN;

void main() {

	v_p_wor = ( u_M * vec4( a_vp, 1.0 ) ).xyz;
	v_st = a_vt;
	v_n_wor = ( u_M * vec4( a_vn, 0.0 ) ).xyz;

	{
		vec3 n_wor         = normalize( v_n_wor );
		vec3 tangent_wor   = normalize( vec3( u_M * vec4( a_vtan, 0.0 ) ) );
		vec3 bitangent_wor = normalize( cross( n_wor, tangent_wor ) ); // * tan.w
		// transpose(TBN) goes from world to tangent space
		// TBN goes from tangent to world space
		TBN = mat3( tangent_wor, bitangent_wor, n_wor );
		v_tan = a_vtan;
	}

	gl_Position = u_P * u_V * vec4( v_p_wor, 1.0 );
}
