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
    float4 shadowPos : POSITION3;
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

SamplerState texSampl : register(s0);
SamplerComparisonState shadowSampl : register(s1);

Texture2D tex : register(t0);
Texture2D depth : register(t1);
Texture2D shadowMap : register(t2);
Texture2D<float> shadowMapFloat : register(t3);

/*
float ShadowCalculation(float4 shadowPos)
{
    // Perform perspective divide (clip space -> NDC)
    float3 projCoords = shadowPos.xyz / shadowPos.w;

    // Transform NDC from [-1,1] to [0,1]
    float2 shadowTexCoords = projCoords.xy * 0.5f + 0.5f;

    // Get current depth from light POV
    float currentDepth = projCoords.z * 0.5f + 0.5f;

    // Sample shadow map depth
    float closestDepth = shadowMap.SampleCmp(shadowSampl, shadowTexCoords, currentDepth + bias);

    // Simple shadow test: 1 if visible, 0 if in shadow
    float shadow = currentDepth > closestDepth + 0.005 ? 0.0f : 1.0f; // +bias to reduce acne

    return shadow;
}
*/

float ShadowCalculation(float4 shadowPos)
{
    // Perform perspective divide (clip space -> NDC)
    float3 projCoords = shadowPos.xyz / shadowPos.w;

    // Transform NDC from [-1,1] to [0,1]
    float2 shadowTexCoords = projCoords.xy * 0.5f + 0.5f;

    // Depth in light space [0,1]
    float currentDepth = projCoords.z * 0.5f + 0.5f;

    // Early exit if outside shadow map bounds (optional)
    if (shadowTexCoords.x < 0.0f || shadowTexCoords.x > 1.0f || shadowTexCoords.y < 0.0f || shadowTexCoords.y > 1.0f)
        return 1.0f;

    // Use SampleCmp with bias to do depth comparison on GPU
    float bias = 0.005f;
    float shadow = shadowMapFloat.SampleCmp(shadowSampl, shadowTexCoords, currentDepth - bias);
    //float shadow = 1.0f;
    
    
    // shadow is 1 if lit, 0 if in shadow, no manual comparison needed here

    return shadow;
}

float4 PSMainOpaque(VertexIn vIn) : SV_TARGET
{
    // Use texturing

    float3 texUV = tex.Sample(texSampl, vIn.uv).xyz;

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

    
    float shadowFactor = ShadowCalculation(vIn.shadowPos);

    //return float4(texUV, 1.0f);
    return float4(shadowFactor.xxx * texUV, 1.0f);
}

float4 PSMainDepthCamera(VertexIn vIn) : SV_TARGET
{
    // Use texturing
    float depthValue = shadowMap.Sample(texSampl, vIn.uv).r;
    //depthValue = pow(depthValue, 32);

    return float4(depthValue.xxx, 1.0f);
}

float4 PSMainShadow(VertexIn vIn) : SV_TARGET
{
    // Use texturing
    float depth = shadowMap.Sample(texSampl, vIn.uv).r;
    
    float4 texUV = tex.Sample(texSampl, vIn.uv);
    //depth = pow(depth, 50);
    return float4(texUV.xyz, 1.0f);
}