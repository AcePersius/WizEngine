struct UniformBuffer
{
    float4x4 model;
    float4x4 view;
    float4x4 proj;
};

ConstantBuffer<UniformBuffer> ubo;

Sampler2D texture;

struct VertexInput
{
    float3 inPos;
    float3 inColor;
    float2 texCoord;
};

struct VertexOutput
{
    float4 pos : SV_Position;
    float3 color;
    float2 texCoord;
};

[shader("vertex")]
VertexOutput vertMain(VertexInput input)
{
    VertexOutput output;
    
    output.pos = mul(ubo.proj, mul(ubo.view, mul(ubo.model, float4(input.inPos, 1.0))));

    output.color = input.inColor;

    output.texCoord = input.texCoord;

    return output;
}

[shader("fragment")]
float4 fragMain(VertexOutput input) : SV_Target
{
    return texture.Sample(input.texCoord);
}