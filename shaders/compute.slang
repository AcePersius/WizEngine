struct ComputeUniformBufferObject
{
    float deltaTime;
};

struct Particle
{
    float2 position;
    float2 velocity;
    float4 color;
};

[[vk::binding(0, 0)]]
ConstantBuffer<ComputeUniformBufferObject> ubo;

[[vk::binding(1, 0)]]
StructuredBuffer<Particle> particlesIn;

[[vk::binding(2, 0)]]
RWStructuredBuffer<Particle> particlesOut;


[shader("compute")]
[numthreads(256, 1, 1)]
void compMain(uint3 threadId : SV_DispatchThreadID)
{
    uint index = threadId.x;

    particlesOut[index].position = particlesIn[index].position + particlesIn[index].velocity * ubo.deltaTime;

    particlesOut[index].velocity = particlesIn[index].velocity;

    particlesOut[index].color = particlesIn[index].color;

    // Bounce off horizontal boundaries
    if (particlesOut[index].position.x <= -1.0 || particlesOut[index].position.x >= 1.0)
    {
        particlesOut[index].velocity.x = -particlesOut[index].velocity.x;
    }

    // Bounce off vertical boundaries
    if (particlesOut[index].position.y <= -1.0 || particlesOut[index].position.y >= 1.0)
    {
        particlesOut[index].velocity.y = -particlesOut[index].velocity.y;
    }
}