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
    float3 worldP : POSITION3;
	float3 col : COLOR;
	float2 uv : UVCOORD;
	float3 normal : NORMAL;
	float3 pixelPos : POSITION;
};

cbuffer ConstantBuffer : register(b0)
{
	float4x4 Model;
    float4x4 World;
    float4x4 LightViewProj;
	Light directionalLight;
    float4 CameraPosition;
}

SamplerState sampl : register(s0);

Texture2D tex : register(t0);
Texture2D depth : register(t1);
Texture2D shadowMap : register(t2);

float3 ShadowTest(float3 worldPos)
{
    // Transform world position to light clip space
    float4 lightSpacePos = mul(float4(worldPos, 1.0f), LightViewProj);
    
    // Perform perspective divide
    lightSpacePos.xyz /= lightSpacePos.w;
    
    // Convert to [0,1] UV space
    float2 shadowUV = lightSpacePos.xy * 0.5f + 0.5f;
    
    // Check if UV is out of bounds (outside light's frustum)
    if (shadowUV.x < 0 || shadowUV.x > 1 || shadowUV.y < 0 || shadowUV.y > 1)
        return 1.0f; // Outside shadow map, assume fully lit

    // Convert light-space z to depth range [0,1]
    float currentDepth = lightSpacePos.z * 0.5f + 0.5f;

    // Sample the shadow map depth
    float shadowMapDepth = shadowMap.Sample(sampl, shadowUV).r;

    // Compare with small bias to avoid shadow acne
    float bias = 0.005f;
    float shadow = (currentDepth - bias) <= shadowMapDepth ? 1.0f : 0.0f;
	
    //return shadow;
	
    float3 litColor = float3(1, 1, 1);
    float3 shadowColor = float3(0.2, 0.2, 0.2);
    float3 finalColor = lerp(shadowColor, litColor, shadow);
    return finalColor;
}

float4 PSMainOpaque(VertexIn vIn) : SV_TARGET
{
	// Use texturing

	float3 texUV = tex.Sample(sampl, vIn.uv).xyz;

	// Use vertex colors
	float3 coloredtexUV = (float4(vIn.col, 1.0f) * texUV).xyz;
	
	float3 coloredPixel = vIn.col;

	float3 ambientLight = float3(0.1f, 0.1f, 0.1f);
	
	// point light
	//float3 pixelToLight = normalize(directionalLight.Position - vIn.pixelPos);

	// directional light
    float3 pixelToLight = -directionalLight.Direction;
	
	float diffuse = max(dot(vIn.normal, pixelToLight), 0.0) * 0.7f;
    float dist = length(directionalLight.Position - vIn.pixelPos);
    float falloffstart = 0.01f;
    float falloffend = 3.0f;
	
    float atten = saturate((falloffend - dist) / (falloffend - falloffstart));
    
	//diffuse *= atten;

	
	// Pixel ---> Camera
	float3 viewDir = normalize(CameraPosition.xyz - vIn.pixelPos);
	
	// Reflect:
	// Pixel ---> Camera
	// Normal face ----->
    float3 reflectedDir = normalize(reflect(-pixelToLight, vIn.normal));
	
    float3 specular = pow(max(dot(reflectedDir, viewDir), 0.0f), 128);

	float3 sampledTexture = (ambientLight + diffuse + specular) * directionalLight.Strength; // use saturate to clamp between 0.0 and 1.0
	sampledTexture *= texUV;
	
	
    //float3 shadowColor = ShadowTest(vIn.worldP);
	
    // Transform current world position to light clip space
    float4 shadowCoord = mul(float4(vIn.worldP, 1.0f), LightViewProj);
    shadowCoord.xyz /= shadowCoord.w;
    shadowCoord.xy = shadowCoord.xy * 0.5f + 0.5f; // NDC to texture space

        // Sample shadow map
    float shadowDepth = shadowMap.Sample(sampl, shadowCoord.xy).r;
    float currentDepth = shadowCoord.z;

    // Apply shadow factor
    float shadow = currentDepth - 0.005f > shadowDepth ? 0.3f : 1.0f;
	
    //float shadowUV = shadowMap.Sample(sampl, vIn.uv);
    
    //return float4(texUV, 1.0f);
    return float4(shadow * texUV, 1.0f);
}

float4 PSMainDepthCamera(VertexIn vIn) : SV_TARGET
{
	// Use texturing
    float depthValue = depth.Sample(sampl, vIn.uv).r;
    depthValue = pow(depthValue, 32);
    
    return float4(depthValue.xxx, 1.0f);
}

float PSMainShadow(VertexIn vIn) : SV_TARGET
{
	// Use texturing
    float depth = shadowMap.Sample(sampl, vIn.uv).r;
    float4 texUV = tex.Sample(sampl, vIn.uv);
    //depth = pow(depth, 50);
    return float4(texUV.xyz, 1.0f);
}