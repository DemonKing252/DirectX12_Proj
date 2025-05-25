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
	float3 Pad0;
	int DrawDepth;
}

SamplerState sampl : register(s0);

Texture2D tex : register(t0);
Texture2D depth : register(t1);

float4 PSMain(VertexIn vIn) : SV_TARGET
{
	// Use texturing

	float3 texUV;
	if (DrawDepth == 0)
		texUV = tex.Sample(sampl, vIn.uv).xyz;
	else
		texUV = depth.Sample(sampl, vIn.uv).xyz;

	// Use vertex colors
	float3 coloredtexUV = (float4(vIn.col, 1.0f) * texUV).xyz;
	
	float3 coloredPixel = vIn.col;

	float3 ambientLight = float3(0.1f, 0.1f, 0.1f);
	float3 pixelToLight = normalize(directionalLight.Position - vIn.pixelPos);

	float diffuse = max(dot(vIn.normal, pixelToLight), 0.0);
    float dist = length(directionalLight.Position - vIn.pixelPos);
    //directionalLight.FallOffStart = 0.1f;
    //directionalLight.FallOffStart = 0.3f;
    float falloffstart = 0.01f;
    float falloffend = 3.0f;
	
    float atten = saturate((falloffend - dist) / (falloffend - falloffstart));
    diffuse *= atten;

	
	// Pixel ---> Camera
	float3 viewDir = normalize(CameraPosition.xyz - vIn.pixelPos);
	
	// Reflect:
	// Pixel ---> Camera
	// Normal face ----->
    float3 reflectedDir = normalize(reflect(pixelToLight, vIn.normal));
	
    float3 specular = pow(max(dot(reflectedDir, viewDir), 0.0f), 32);

	float3 sampledTexture = (ambientLight + diffuse) * directionalLight.Strength; // use saturate to clamp between 0.0 and 1.0
	sampledTexture *= texUV;

	if (DrawDepth == 1)
		return float4(texUV, 1.0f);
	else
		return float4(sampledTexture, 1.0f);
}