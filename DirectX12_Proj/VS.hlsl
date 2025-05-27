
struct VertexOut
{
    float4 pos : SV_POSITION;
    float3 worldP : POSITION3;
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
    float4x4 LightViewProj;
	Light directionalLight;
    float4 CameraPosition;
}

VertexOut VSMainOpaque(float3 p : POSITION, float3 c : COLOR, float2 uv : UVCOORD, float3 n : NORMAL)
{
	VertexOut vOut;
	
    float4 WorldP = mul(float4(p, 1.0f), World);
    
    //float4 WorldPModel = mul(float4(p, 1.0f), Model);
    //float4 WorldP = mul(WorldPModel, LightViewProj);
    //float4 WorldP = mul(WorldPModel, LightViewProj); 
    //float4 WorldP = mul(float4(p, 1.0f), LightViewProj);
    //float4 LightP = mul(WorldP, LightViewProj);
    //float4 LightP = mul(float4(p, 1.0f), LightViewProj);

    vOut.worldP = WorldP.xyz;
    vOut.pos = WorldP;
	vOut.col = c;
	vOut.uv = uv;
    vOut.pixelPos = mul(float4(p, 1.0f), Model);
	//vOut.normal = n;
	vOut.normal = normalize(mul(n, (float3x3)Model));

	return vOut;
}

VertexOut VSMainDepthCamera(float3 p : POSITION, float3 c : COLOR, float2 uv : UVCOORD, float3 n : NORMAL)
{
    VertexOut vOut;
	
    float4 WorldP = float4(p, 1.0f);

    vOut.pos = WorldP;
    vOut.col = c;
    vOut.uv = uv;
    vOut.pixelPos = mul(float4(p, 1.0f), Model);
    vOut.normal = normalize(mul(n, (float3x3) Model));

    return vOut;
}

VertexOut VSMainShadow(float3 p : POSITION, float3 c : COLOR, float2 uv : UVCOORD, float3 n : NORMAL)
{
    VertexOut vOut;
    
    //float4 WorldP = mul(float4(p, 1.0f), Model);
    //float4 WorldP = mul(float4(p, 1.0f), World);
    //float4 WorldPos = mul(WorldP, LightViewProj);
    float4 WorldP = mul(float4(p, 1.0f), LightViewProj);
    //float4 WorldP = mul(LightViewProj, float4(p, 1.0f));

    vOut.worldP = WorldP;
    vOut.pos = WorldP;
    vOut.col = c;
    vOut.uv = uv;
    vOut.pixelPos = mul(float4(p, 1.0f), Model);
    vOut.normal = normalize(mul(n, (float3x3) Model));

    return vOut;
}