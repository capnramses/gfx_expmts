// cubemap frag shader that convolutes an environment map into an irradiance map
// based on: https://github.com/JoeyDeVries/LearnOpenGL/blob/master/src/6.pbr/2.1.2.ibl_irradiance/2.1.2.irradiance_convolution.fs
#version 410 core

in vec3 v_st;
uniform samplerCube u_texture_a;
out vec4 o_frag_colour;

const float PI = 3.14159265359;

void main () {
	vec3 n_wor = normalize( v_st );

  // tanget space from origin point
  vec3 up    = vec3( 0.0, 1.0, 0.0 );
  vec3 right = cross( up, n_wor );
  up         = cross( n_wor, right );

  float sample_delta = 0.025; // decrease to increase accuracy
  float n_samples = 0.0;
  vec3 irradiance = vec3( 0.0 );
  for ( float phi = 0.0; phi < 2.0 * PI; phi += sample_delta ) {
    for ( float theta = 0.0; theta < 0.5 * PI; theta += sample_delta ) {
      // spherical to cartesian
      vec3 sample_tan = vec3( sin( theta ) * cos( phi ), sin( theta ) * sin( phi ), cos( theta ) );
      // tangent space to world
      vec3 sample_wor  = sample_tan.x * right + sample_tan.y * up + sample_tan.z * n_wor;

      irradiance += texture( u_texture_a, sample_wor ).rgb * cos( theta ) * sin( theta ); // weights to compensate for intensity and smaller area at higher altitude
      //irradiance += texture( u_texture_a, n_wor ).rgb;
      n_samples += 1.0;
    }
  }
  irradiance *= PI * ( 1.0 / n_samples );
  //irradiance /= n_samples;

 // irradiance = texture( u_texture_a, n_wor ).rgb;
  
	o_frag_colour.rgb = irradiance.rgb;
	o_frag_colour.a = 1.0;
}
