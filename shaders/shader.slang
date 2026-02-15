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

struct PC {
    row_major float4x4 model;
};

[[vk_push_constant]]
ConstantBuffer<PC> pc;

struct Uniform {
    row_major float4x4 view;
    row_major float4x4 projection;
};

[[vk_binding(0, 0)]]
ConstantBuffer<Uniform> ubo;

[[vk_binding(0, 1)]]
Sampler2D sampledTexture;

[shader("vertex")]
VertexOutput vertMain(VertexInput vIn) {
    VertexOutput vOut;
    vOut.pos = mul(ubo.projection, (mul(ubo.view, mul(pc.model, float4(vIn.pos, 1.f)))));
    // vOut.pos = float4(vIn.pos, 1.f);
    vOut.color = vIn.color;
    vOut.uv = vIn.uv;
    return vOut;
}

[shader("fragment")]
float4 fragMain(VertexOutput fIn) : SV_Target {
    return sampledTexture.Sample(fIn.uv);
    // return float4(1.f, 0.f, 0.f, 0.5f);
}


