//
// http://stackoverflow.com/questions/10786951/omnidirectional-shadow-mapping-with-depth-cubemap
//

#version 410

in vec3 dist_of_light;
// note shadow
uniform samplerCubeShadow depth_map;
uniform vec3 colour;
out vec4 frag_colour;

float VectorToDepthValue(vec3 Vec)
{
    vec3 AbsVec = abs(Vec);
    float LocalZcomp = max(AbsVec.x, max(AbsVec.y, AbsVec.z));

    const float f = 1000.0;
    const float n = 1.0;
    float NormZComp = (f+n) / (f-n) - (2.0*f*n)/(f-n)/LocalZcomp;
    return (NormZComp + 1.0) * 0.5;
}

float ComputeShadowFactor(samplerCubeShadow ShadowCubeMap, vec3 VertToLightWS)
{   
    float ShadowVec = texture(ShadowCubeMap, vec4(VertToLightWS, 1.0));
    if (ShadowVec + 0.00001 > VectorToDepthValue(VertToLightWS))
        return 1.0;

    return 0.0;
}
/* my original frankenstein wrong one but almost there!
float eval_shadow () {
	vec3 v = (dist_of_light);
	float cubes_dist = textureCube (depth_map, v).r;
	float dist_wor = length (v);

	float n = 1.0f;
	float f = 50.0f;
	float z = 2.0 * n * f / (f + n - cubes_dist * (f - n));
	
	float dn = 2.0 * z - 1.0;
	dn = (dn - n) / (f - n);
	dist_wor = (dist_wor - n) / (f - n);
	
	float epsilon = -0.5;
	if (dist_wor < dn + epsilon) {
		return 1.0;
	} else {
		return 0.5;
	}
}*/

void main() {
	frag_colour = vec4 (colour, 1.0);
	//float shadow_factor = eval_shadow ();
	
	float shadow_factor = ComputeShadowFactor (depth_map, dist_of_light);
	
	
	frag_colour = vec4 (colour * shadow_factor, 1.0);
}
