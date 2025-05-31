
struct VertexOut
{
    float4 pos : SV_POSITION;
    float4 shadowPos : POSITION3;
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
    float4x4 PerspectiveViewProj;
    Light directionalLight;
    float4 CameraPosition;
}

VertexOut VSMainOpaque(float3 p : POSITION, float3 c : COLOR, float2 uv : UVCOORD, float3 n : NORMAL)
{
    VertexOut vOut;
    
    //float4 WorldP = mul(float4(p, 1.0f), World);
    float4 ModelP = mul(float4(p, 1.0f), Model);
    
    vOut.shadowPos = mul(ModelP, LightViewProj);
    vOut.pos = mul(ModelP, PerspectiveViewProj);
    
    vOut.col = c;
    vOut.uv = uv;
    vOut.pixelPos = mul(float4(p, 1.0f), Model);
    vOut.normal = normalize(mul(n, (float3x3)Model));

    return vOut;
}
//float4 WorldP = mul(float4(p, 1.0f), World);
//vOut.shadowPos = mul(float4(p, 1.0f), Model);
VertexOut VSMainDepthCamera(uint vertexID : SV_VertexID)
{
    // We don't really need a vertex buffer for this because we are drawing a simple quad with no World matrix.

    float2 vertexInner = float2(0.5f, -0.5f);
    float2 vertexOuter = float2(1.0f, -1.0f);
    
    float3 vertex_positions[] = {
        float3(vertexInner.x, vertexInner.y, 0.0f),
        float3(vertexOuter.x, vertexInner.y, 0.0f),
        float3(vertexOuter.x, vertexOuter.y, 0.0f),
        float3(vertexOuter.x, vertexOuter.y, 0.0f),
        float3(vertexInner.x, vertexOuter.y, 0.0f),
        float3(vertexInner.x, vertexInner.y, 0.0f),
    };

    float2 uv_coords[] = {
        float2(0.0f, 0.0f),
        float2(1.0f, 0.0f),
        float2(1.0f, 1.0f),
        float2(1.0f, 1.0f),
        float2(0.0f, 1.0f),
        float2(0.0f, 0.0f),
    
    };
    VertexOut vout;
    vout.shadowPos = float4(vertex_positions[vertexID], 1.0f);
    vout.pos = float4(vertex_positions[vertexID], 1.0f);
    vout.uv = uv_coords[vertexID];
    
    return vout;
}

VertexOut VSMainShadow(float3 p : POSITION, float3 c : COLOR, float2 uv : UVCOORD, float3 n : NORMAL)
{
    VertexOut vOut;
    
    //float4 ModelP = mul(float4(p, 1.0f), Model);
    //float4 WorldP = mul(World, float4(p, 1.0f)); // Model * position
    float4 ModelP = mul(float4(p, 1.0f), Model); // (if you use Model separately)
    //float4 lightP = mul(float4(p, 1.0f), LightViewProj); // LightViewProj * ModelP
    
    /*
    float4 WorldP = mul(float4(p, 1.0f), World);
    float4 ModelP = mul(float4(p, 1.0f), Model);
    float4 lightP = mul(ModelP, LightViewProj);
    */

    //vOut.worldP = WorldP;
    vOut.pos = mul(ModelP, LightViewProj);
    vOut.col = c;
    vOut.uv = uv;
    vOut.pixelPos = mul(float4(p, 1.0f), Model);
    vOut.shadowPos = mul(ModelP, LightViewProj);
    vOut.normal = normalize(mul(n, (float3x3) Model));
    //vOut.shadowPos = mul(float4(p, 1.0f), Model);
    return vOut;
}

VertexOut VSMainOutline(float3 p : POSITION, float3 c : COLOR, float2 uv : UVCOORD, float3 n : NORMAL)
{
    VertexOut vOut;
    
    float4 WorldPModel = mul(float4(p, 1.0f), Model);
    vOut.shadowPos = mul(WorldPModel, LightViewProj);
    vOut.pos = mul(float4(p, 1.0f), World);
    vOut.col = c;
    vOut.uv = uv;
    vOut.pixelPos = mul(float4(p, 1.0f), Model);
    vOut.normal = normalize(mul(n, (float3x3) Model));

    return vOut;
}