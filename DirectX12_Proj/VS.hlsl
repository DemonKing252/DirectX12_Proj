#pragma pack_matrix(row_major)

struct VertexOut
{
	float4 pos : SV_POSITION;
	float3 col : COLOR;
	float2 uv : UVCOORD;
	float3 normal : NORMAL;
};

struct Light
{
	float3 Direction;
    float pad1;
	float3 Strength;
    float pad2;
};

cbuffer ConstantBuffer : register(b0)
{
	float4x4 Model;
	float4x4 World;
	Light directionalLight;
}

VertexOut VSMain(float3 p : POSITION, float3 c : COLOR, float2 uv : UVCOORD, float3 n : NORMAL)
{
	VertexOut vOut;
	
	float4 WorldP = mul(World, float4(p, 1.0f));

	vOut.pos = WorldP;
	vOut.col = c;
	vOut.uv = uv;
	vOut.normal = normalize(mul((float3x3)Model, n));

	return vOut;
}