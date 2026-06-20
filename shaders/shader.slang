struct VertexInput {
    float3 pos;
    float4 color;
    float2 uv;
};

struct VertexOutput {
    float4 pos : SV_Position;
    float4 color;
    float2 uv;
};

[[vk_binding(0, 0)]]
Sampler2D sampledTexture;

[[vk_push_constant]]
ConstantBuffer<float4x4> transform;

[shader("vertex")]
VertexOutput vertMain(VertexInput vIn) {
    return VertexOutput(mul(transform, float4(vIn.pos, 1.f)), vIn.color, vIn.uv);
}

[shader("fragment")]
float4 fragMain(VertexOutput fIn) : SV_Target {
    float4 value = sampledTexture.Sample(fIn.uv);
    if (value.r > 0) {
        return float4(1.0, 1.0, 1.0, 1.0);
    }
    else {
        return float4(0.0, 0.0, 0.0, 1.0);
    }
}

