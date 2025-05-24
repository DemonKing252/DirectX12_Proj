struct Light
{
    float3 Position;
    float FallOffStart;
    float3 Direction;
    float FallOffEnd;
	float3 Strength;
    int type;	// 1 for directional, 2 for point light, 3 for spot light
};

struct VertexIn
{
	float4 pos : SV_POSITION;
	float3 col : COLOR;
	float2 uv : UVCOORD;
	float3 normal : NORMAL;
	float3 pixelPos : POSITION;
};

cbuffer ConstantBuffer : register(b0)
{
	float4x4 Model;
	float4x4 World;
	Light directionalLight;
	float4 CameraPosition;
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

	float diffuse = max(dot(vIn.normal, normalize(-directionalLight.Direction)), 0.0);
    float dist = length(directionalLight.Position - vIn.pixelPos);
    //directionalLight.FallOffStart = 0.1f;
    //directionalLight.FallOffStart = 0.3f;
    float falloffstart = 0.5999f;
    float falloffend = 0.6f;
	
    float atten = saturate((falloffend - dist) / (falloffend - falloffstart)) * 1.5f;
    diffuse *= atten;

	
	// Light to Fragment Vector
	float3 viewDir = normalize(CameraPosition.xyz - vIn.pixelPos);
	
    float3 reflectedDir = normalize(reflect(-directionalLight.Direction, vIn.normal));
	
    float3 specular = pow(max(dot(reflectedDir, viewDir), 0.0f), 1024 * 8);

	float3 sampledTexture = (ambientLight + diffuse) * directionalLight.Strength; // use saturate to clamp between 0.0 and 1.0
	sampledTexture *= texUV;
	
	return float4(sampledTexture, 1.0f);
}