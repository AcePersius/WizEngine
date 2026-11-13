struct ParticleVertexInput
{
    [[vk::location(0)]]
    float2 position;

    [[vk::location(1)]]
    float4 color;
};

// Structure strictly for Vertex Shader Output / Rasterizer Input
struct ParticleVertexOutput
{
    float4 position : SV_Position;

    [[vk::location(0)]]
    float4 color;

    float pointSize : SV_PointSize;
};

// Structure strictly for Fragment Shader Input
struct ParticleFragmentInput
{
    float4 position : SV_Position;

    [[vk::location(0)]]
    float4 color;
};


[shader("vertex")]
ParticleVertexOutput particleVertMain(ParticleVertexInput input)
{
    ParticleVertexOutput output;

    output.position = float4(
        input.position.x,
        input.position.y,
        0.0,
        1.0
    );

    output.color = input.color;

    // Keep this at 1.0 for now.
    // Larger values can require the widePoints device feature.
    output.pointSize = 1.0;

    return output;
}


[shader("fragment")]
float4 particleFragMain(ParticleFragmentInput input) : SV_Target
{
    return input.color;
}