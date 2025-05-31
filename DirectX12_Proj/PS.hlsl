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
    float4 shadowPos : SHADOW_POS;
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
    float4x4 LightViewProjTextureSpace;
    float4x4 PerspectiveViewProj;
    Light directionalLight;
    float4 CameraPosition;
}

SamplerState texSampl : register(s0);
SamplerComparisonState shadowSampl : register(s1);

Texture2D tex : register(t0);
Texture2D<float> depth : register(t1);
Texture2D<float> shadowMap : register(t2);

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

float4 ComputeShadowFactor(float4 shadowPos)
{
    // Perform perspective divide (clip space -> NDC)
    shadowPos.xyz /= shadowPos.w;
    float depth = shadowPos.z;
    
    // Transform NDC from [-1,1] to [0,1]
    //float2 shadowTexCoords = shadowPos.xy * 0.5f + 0.5f;

    // Depth in light space [0,1]
    
    //float currentDepth = shadowPos.z * 0.5f + 0.5f;

    // Early exit if outside shadow map bounds (optional)
    
    //if (shadowTexCoords.x < 0.0f || shadowTexCoords.x > 1.0f || shadowTexCoords.y < 0.0f || shadowTexCoords.y > 1.0f)
    //    return 1.0f;

    // Use SampleCmp with bias to do depth comparison on GPU
    float bias = 0.0001f;
    
    //float shadow = shadowMapFloat.SampleCmp(shadowSampl, shadowTexCoords, currentDepth - bias);
    
    float far = 15.0f;
    float near = 0.01f;
    
    float rawDepth = shadowMap.Sample(texSampl, shadowPos.xy).r;
    float texelSize = 1.0f / 1024.0f;
    //float z = rawDepth * 2.0 - 1.0; // NDC space
    //float linearDepth = (2.0 * near * far) / (far + near - z * (far - near));
    //return float4(linearDepth.xxx, 1.0f);
    
    //float shadow = shadowMap.SampleCmpLevelZero(shadowSampl, shadowPos.xy, depth).r;
    //float shadowDepth = shadowMap.SampleCmp(shadowSampl, shadowPos.xy, depth);
    float shadowDepth = 0.0f;
    
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float2 offset = float2(x, y) * texelSize;
            shadowDepth += shadowMap.SampleCmp(shadowSampl, shadowPos.xy + offset, depth - bias);
        }
    }
    shadowDepth /= 9.0f;
            
    
    //float shadowDepth = shadowMap.SampleCmpLevelZero(shadowSampl, shadowTexCoords, saturate(currentDepth - bias)).r;
    
    //float depthFromShadowMap = shadowMap.SampleLevel(texSampl, shadowTexCoords, 0).r;
    //float shadow = shadowMap.SampleCmpLevelZero(shadowSampl, shadowTexCoords, currentDepth - bias);
    //float depthDelta = currentDepth - depthFromShadowMap;
    //rawDepth = shadowMap.SampleCmpLevelZero(shadowSampl, shadowTexCoords, currentDepth).r;

    
    //float shadow = (depth - bias > shadowDepth) ? 0.2f : 1.0f;
    
    //float shadow = (shadowDepthRaw < 20.0f / 255.0f) ? 0.2f : 1.0f;
    
    // shadow is 1 if lit, 0 if in shadow, no manual comparison needed here

    
    
    return float4(shadowDepth.xxx, 1.0f);
}
float3 ComputeLighting(float3 normal, float3 pixelPos)
{
    float3 ambientLight = float3(0.2f, 0.2f, 0.2f);
    
    // point light
    //float3 pixelToLight = normalize(directionalLight.Position - vIn.pixelPos);

    // directional light
    float3 pixelToLight = -directionalLight.Direction;

    
    float diffuse = saturate(max(dot(normal, pixelToLight), 0.0)) * 0.7f;
    float dist = length(directionalLight.Position - pixelPos);
    float falloffstart = 0.01f;
    float falloffend = 3.0f;

    float atten = saturate((falloffend - dist) / (falloffend - falloffstart));

    //diffuse *= atten;


    // Pixel ---> Camera
    float3 viewDir = normalize(CameraPosition.xyz - pixelPos);

    // Reflect:
    // Pixel ---> Camera
    // Normal face ----->
    float3 reflectedDir = normalize(reflect(-pixelToLight, normal));
    
    float3 specular = saturate(pow(max(dot(reflectedDir, viewDir), 0.0f), 32));

    float3 totalLight = (ambientLight + diffuse + specular); // use saturate to clamp between 0.0 and 1.0
    
    return totalLight;
}

float4 PSMainOpaque(VertexIn vIn) : SV_TARGET
{
    // Use texturing

    float3 texUV = tex.Sample(texSampl, vIn.uv).xyz;

    // Use vertex colors
    //float3 coloredtexUV = (float4(vIn.col, 1.0f) * texUV).xyz;
    //float3 coloredPixel = vIn.col;
    
    //shadowFactor = (shadowFactor < 0.15f ? 0.2f : 1.0f);
    //shadowFactor = max(shadowFactor, 0.2f);
    
    //shadowFactor = (vIn.shadowPos > 2.5f && vIn.shadowPos < 3.0f ? 1.0f : 0.2f);
    //return float4(texUV, 1.0f);
    float3 sampledTexture = ComputeLighting(vIn.normal, vIn.pixelPos) * texUV;
    
    //float3 finalComputePixelColor = (shadowFactor < 0.5f ? 0.3f * texUV : sampledTexture);
    //float3 finalComputePixelColor = shadowFactor * sampledTexture;
    
    //finalComputePixelColor = float3(shadowFactor, shadowFactor, shadowFactor) * sampledTexture;
    //float3 finalShadow;
    //if (shadowFactor < (19.0f / 255.0f))
    //    finalShadow = float3(0.2, 0.2f, 0.2f);
    //else
    //    finalShadow = float3(1.0f, 1.0f, 1.0f);
    
    
    
    float shadowFactor = max(ComputeShadowFactor(vIn.shadowPos), 0.4f);
    //float shadowFactor = ComputeShadowFactor(vIn.shadowPos);
    //shadowFactor = (length(sampledTexture) < 0.3f ? 0.3f : 1.0f);
    
    return float4(texUV * shadowFactor.xxx, 1.0f);
}

/*
float PSMainOpaque(VertexIn vIn) : SV_Depth
{
    return vIn.pos.z / vIn.pos.w;
}
*/

float4 PSMainDepthCamera(VertexIn vIn) : SV_TARGET
{
    // Use texturing
    float4 depthValue = shadowMap.Sample(texSampl, vIn.uv);
    
    //float depth = shadowMap.SampleCmpLevelZero(shadowSampl, vIn.uv, 0.5f);
    
    //depthValue = pow(depthValue, 4);

    //return 1.0f;
    return float4(depthValue.rrr, 1.0f);
}


float PSMainShadow(VertexIn vIn) : SV_Depth
{
    //return 0.0f;
    return vIn.shadowPos.z / vIn.shadowPos.w;
}



//float4 PSMainShadow(VertexIn vIn) : SV_Target
//{
//    float4 depthValue = shadowMap.Sample(texSampl, vIn.uv).r;
//    return float4(depthValue.xxx, 1.0f);
//}


float4 PSMainOutline(VertexIn vIn) : SV_TARGET
{
    return float4(1.0f, 0.7f, 0.0f, 1.0f);
}