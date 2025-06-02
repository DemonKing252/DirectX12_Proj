struct Light
{
    float3 Position;
    float FallOffStart;
    float3 Direction;
    float FallOffEnd;
    float3 Strength;
    int type;	// 1 for directional, 2 for point light, 3 for spot light
};

struct LightResult
{
    float3 Ambient;
    float3 Diffuse;
    float Specular;
};

struct VertexIn
{
    float4 pos : SV_POSITION;
    float4 shadowPos : SHADOW_POS;
    float3 col : COLOR;
    float2 uv : UVCOORD;
    float3 normal : NORMAL;
    float3 pixelPos : POSITION;
    float3 cubeUV : CUBEUV;
};

cbuffer ConstantBuffer : register(b0)
{
    float4x4 Model;
    float4x4 World;
    float4x4 LightViewProj;
    float4x4 LightViewProjTextureSpace;
    float4x4 PerspectiveViewProj;
    float4x4 View;
    float4x4 Proj;
    Light directionalLight;
    float4 CameraPosition;
    float3 garbagePadding;
    float schilickFresenel;
}

SamplerState texSampl : register(s0);
SamplerComparisonState shadowSampl : register(s1);

Texture2D tex : register(t0);
Texture2D<float> depth : register(t1);
Texture2D<float> shadowMap : register(t2);

TextureCube reflectMap : register(t3);



TextureCube skyboxCube : register(t4);
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
    
    // Early exit if outside shadow map bounds
    if (shadowPos.x < 0.0f || shadowPos.x > 1.0f || shadowPos.y < 0.0f || shadowPos.y > 1.0f)
        return 1.0f;
    
    //float bias = 0.0001f;

    float rawDepth = shadowMap.Sample(texSampl, shadowPos.xy).r;
    float texelSize = 1.0f / 1024.0f;

    float shadowDepth = 0.0f;
    
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float2 offset = float2(x, y) * texelSize;
            shadowDepth += shadowMap.SampleCmp(shadowSampl, shadowPos.xy + offset, depth);
        }
    }
    shadowDepth /= 9.0f;
     
    return float4(shadowDepth.xxx, 1.0f);
}
LightResult ComputeLighting(float3 normal, float3 pixelPos)
{
    float3 ambientLight = float3(0.2f, 0.2f, 0.2f);
    
    float3 pixelToLight = -directionalLight.Direction;

    
    float diffuse = saturate(max(dot(normal, pixelToLight), 0.0)) * 0.7f;
    float dist = length(directionalLight.Position - pixelPos);
    float falloffstart = 0.01f;
    float falloffend = 3.0f;

    float atten = saturate((falloffend - dist) / (falloffend - falloffstart));
    
    float3 viewDir = normalize(CameraPosition.xyz - pixelPos);
    
    float3 reflectedDir = normalize(reflect(-pixelToLight, normal));
    
    float3 specular = saturate(pow(max(dot(reflectedDir, viewDir), 0.0f), 32));
    
    LightResult lResult;
    lResult.Ambient = ambientLight;
    lResult.Diffuse = diffuse.xxx;
    lResult.Specular = specular.xxx;
    
    return lResult;
}

float4 PSMainOpaque(VertexIn vIn) : SV_TARGET
{
    
    LightResult light_result = ComputeLighting(vIn.normal, vIn.pixelPos);
    
    float3 lightResult = saturate(light_result.Ambient + light_result.Diffuse + light_result.Specular);
    float3 texUV = tex.Sample(texSampl, vIn.uv).xyz;
    
    float shadowFactor = max(ComputeShadowFactor(vIn.shadowPos), 0.4f);
    
    if (light_result.Diffuse.x < 0.1f)
        shadowFactor = 0.4f;
    
    // Disable Diffuse & Specular light if shadow appears
    if (shadowFactor < 0.5f)
        lightResult = float3(0.6f, 0.6f, 0.6f);
        
    
    float3 returnColor = shadowFactor.xxx * lightResult * texUV;
    
    
    float3 viewDirSkybox = normalize(CameraPosition.xyz - vIn.pixelPos.xyz);
    float3 reflectedDirSkybox = reflect(-viewDirSkybox, normalize(vIn.normal));
    //reflectedDirSkybox.xy *= 0.3f;
    reflectedDirSkybox = normalize(reflectedDirSkybox);
    float3 reflectColor = skyboxCube.Sample(texSampl, reflectedDirSkybox);
    
    //float reflectiveness = 0.9;
    float3 finalColor = lerp(returnColor, reflectColor, schilickFresenel);
    
    return float4(finalColor, 1.0f);
}

float4 PSMainDepthCamera(VertexIn vIn) : SV_TARGET
{
    // Use texturing
    
    float3 dirX;
    dirX.x = 1.0f; // +X face
    dirX.y = 1.0f - 2.0f * vIn.uv.y; // Y mapped [-1, 1]
    dirX.z = 2.0f * vIn.uv.x - 1.0f; // Z mapped [-1, 1]
    
    dirX = normalize(dirX);
    
    float4 depthValue = shadowMap.Sample(texSampl, vIn.uv);
    
    float4 colorValue = reflectMap.Sample(texSampl, dirX);
    //float4 colorValue = reflectMap.Sample(texSampl, vIn.uv);
    
    return float4(colorValue.xyz, 1.0f);
}


float PSMainShadow(VertexIn vIn) : SV_Depth
{
    //return 0.0f;
    return vIn.shadowPos.z / vIn.shadowPos.w;
}

float4 PSMainOutline(VertexIn vIn) : SV_TARGET
{
    return float4(1.0f, 0.7f, 0.0f, 1.0f);
}


//float3 SampleTexture3D(float3 worldPos, float3 normal, Texture2D tex)
//{
//    // Take absolute normal components for blending weights
//    float3 blendWeights = abs(normal);
//    blendWeights /= (blendWeights.x + blendWeights.y + blendWeights.z);
//
//    // Project world position onto three planes:
//    float2 uvX = worldPos.yz; // projection on YZ plane for X axis
//    float2 uvY = worldPos.xz; // projection on XZ plane for Y axis
//    float2 uvZ = worldPos.xy; // projection on XY plane for Z axis
//
//    // Sample texture on each plane:
//    float3 sampleX = reflectMap.Sample(texSampl, uvX).rgb;
//    float3 sampleY = reflectMap.Sample(texSampl, uvY).rgb;
//    float3 sampleZ = reflectMap.Sample(texSampl, uvZ).rgb;
//
//    // Blend samples based on normal direction weights:
//    return sampleX * blendWeights.x + sampleY * blendWeights.y + sampleZ * blendWeights.z;
//}

float4 PSReflect(VertexIn vIn) : SV_TARGET
{
    LightResult light_result = ComputeLighting(vIn.normal, vIn.pixelPos);
    
    float3 lightResult = saturate(light_result.Ambient + light_result.Diffuse + light_result.Specular);
    
    
    
    float shadowFactor = max(ComputeShadowFactor(vIn.shadowPos), 0.4f);
    
    if (light_result.Diffuse.x < 0.1f)
        shadowFactor = 0.4f;
    
    // Disable Diffuse & Specular light if shadow appears
    if (shadowFactor < 0.5f)
        lightResult = float3(0.6f, 0.6f, 0.6f);
        
    float3 texUV = tex.Sample(texSampl, vIn.uv).xyz;
    //float3 returnColor = shadowFactor.xxx * lightResult * texUV;
    
    
    float3 viewDir = normalize(CameraPosition.xyz - vIn.pixelPos);
    float3 reflectedDir = reflect(-viewDir, normalize(vIn.normal));
    
    float sx = 1.0f; // fixed major axis for +X face
    float sy = 1.0f - 2.0f * vIn.uv.x; // invert u for y-axis
    float sz = 1.0f - 2.0f * vIn.uv.y; // invert v for z-axis
    
    
    float3 viewDirSkybox = normalize(CameraPosition.xyz - vIn.pixelPos.xyz);
    float3 reflectedDirSkybox = reflect(-viewDirSkybox, normalize(vIn.normal));
    //reflectedDirSkybox.xy *= 0.5f;
    reflectedDirSkybox = normalize(reflectedDirSkybox);
    
    //float3 reflectiveMap = reflectMap.Sample(texSampl, vIn.uv).xyz;
    float3 reflectiveMap = reflectMap.Sample(texSampl, reflectedDirSkybox).xyz;
    //float3 reflectiveMap = reflectMap.Sample(texSampl, float3(sx, sy, sz)).xyz;
    //
    
    //float2 tiledUV = vIn.uv * float2(2.0f, 2.0f);
    
    //float3 reflectiveMap = reflectMap.Sample(texSampl, vIn.uv).xyz;
    
    
     // glassy but not mirror-like
    //float fresnel = pow(1.0 - saturate(dot(viewDir, vIn.normal)), 5.0);
    
    float3 finalColor = lerp(texUV, reflectiveMap, schilickFresenel);
    //float3 finalColor = SampleTexture3D(vIn.pixelPos.xyz, normalize(vIn.normal), tex);
    
    
       
    
    //float3 reflectColor = skyboxCube.Sample(texSampl, reflectedDirSkybox);
    //float3 texUV = tex.Sample(texSampl, vIn.uv).xyz;
    //float3 finalColor = lerp(texUV, reflectColor, schilickFresenel);
    
    
    return float4(finalColor, 1.0f);
}

float4 PSSkybox(VertexIn vIn) : SV_TARGET
{
    
    LightResult light_result = ComputeLighting(vIn.normal, vIn.pixelPos);
    
    float3 lightResult = saturate(light_result.Ambient + light_result.Diffuse + light_result.Specular);
    float3 texUV = tex.Sample(texSampl, vIn.uv).xyz;
    
    float shadowFactor = max(ComputeShadowFactor(vIn.shadowPos), 0.4f);
    
    if (light_result.Diffuse.x < 0.1f)
        shadowFactor = 0.4f;
    
    // Disable Diffuse & Specular light if shadow appears
    if (shadowFactor < 0.5f)
        lightResult = float3(0.6f, 0.6f, 0.6f);
        
    
    float3 returnColor = shadowFactor.xxx * lightResult * texUV;
    
    float3 skyBoxUV = skyboxCube.Sample(texSampl, vIn.cubeUV);
    
    return float4(skyBoxUV, 1.0f);
}