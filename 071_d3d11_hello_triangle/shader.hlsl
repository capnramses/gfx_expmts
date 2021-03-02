
struct VS_INPUT
{
    float3 vPosition    : POSITION;
    float3 vColour        : COLOR;
};

struct VS_OUTPUT
{
    float3 vColour        : COLOR;
    float4 vPosition    : SV_POSITION;
};

VS_OUTPUT VSMain( VS_INPUT Input )
{
    VS_OUTPUT Output;
    
    Output.vPosition = float4(Input.vPosition.xy, 0.0, 1.0);
    Output.vColour = Input.vColour;
    
    return Output;
}

float4 PSMain( VS_OUTPUT Input ) : SV_TARGET
{
    return float4(Input.vColour, 1.0);
}
