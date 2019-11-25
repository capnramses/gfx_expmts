/* vertex attributes go here to input to the vertex shader */
struct vs_in
{
    float3 position_local : POS;
};

/* outputs from vertex shader go here. can be interpolated to pixel shader */
struct vs_out
{
    float4 position_clip : SV_POSITION; // required output of VS
};


vs_out vs_main(vs_in input) {
  vs_out output;
  output.position_clip = float4(input.position_local, 1.0);
  return output;
}

float4 ps_main(vs_out input) : SV_TARGET {
  return float4( 1.0, 0.0, 1.0, 1.0 ); // must return an RGBA colour
}