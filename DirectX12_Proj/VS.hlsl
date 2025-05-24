
struct VertexOut
{
	float4 pos : SV_POSITION;
	float3 col : COLOR;
	float2 uv : UVCOORD;
	float3 normal : NORMAL;
	float3 pixelPos : POSITION;
};

struct Light
{
    
    float3 Position;
    float FallOffStart;
    float3 Direction;
    float FallOffEnd;
    float3 Strength;
    int type; // 1 for directional, 2 for point light, 3 for spot light
};

cbuffer ConstantBuffer : register(b0)
{
	float4x4 Model;
	float4x4 World;
	Light directionalLight;
	float4 CameraPosition;
}

VertexOut VSMain(float3 p : POSITION, float3 c : COLOR, float2 uv : UVCOORD, float3 n : NORMAL)
{
	VertexOut vOut;
	
	float4 WorldP = mul(float4(p, 1.0f), World);

	vOut.pos = WorldP;
	vOut.col = c;
	vOut.uv = uv;
    vOut.pixelPos = normalize(mul(p, (float3x3)Model));
	//vOut.normal = n;
	vOut.normal = normalize(mul(n,  (float3x3)Model));

	return vOut;
}