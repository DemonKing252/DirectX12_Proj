#pragma pack_matrix(row_major)
struct Light
{
	float3 Direction;
    float pad1;
	float3 Strength;
    float pad2;
};

struct VertexIn
{
	float4 pos : SV_POSITION;
	float3 col : COLOR;
	float2 uv : UVCOORD;
	float3 normal : NORMAL;
};

cbuffer ConstantBuffer : register(b0)
{
	float4x4 Model;
	float4x4 World;
	Light directionalLight;
}

SamplerState sampl : register(s0);
Texture2D tex : register(t0);

float4 PSMain(VertexIn vIn) : SV_TARGET
{
	// Use texturing
	float3 texUV = tex.Sample(sampl, vIn.uv).xyz;

	// Use vertex colors
	float3 coloredtexUV = (float4(vIn.col, 1.0f) * texUV).xyz;

	float3 coloredPixel = vIn.col;

	float3 ambientLight = float3(0.1f, 0.1f, 0.1f);

	float lightFactor = max(dot(vIn.normal, normalize(-directionalLight.Direction)), 0.0);

	float3 lightStrength = float3(lightFactor.xxx) * directionalLight.Strength;

	float3 sampledTexture = ambientLight + lightStrength; // use saturate to clamp between 0.0 and 1.0
	sampledTexture *= texUV.xyz;
	
	return float4(sampledTexture, 1.0f);
}