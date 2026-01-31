struct VertexInput {
    float3 pos : POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};

struct VertexOutput {
    float4 pos : SV_Position;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};

struct PC {
    row_major float4x4 model;
};

[[vk::push_constant()]]
ConstantBuffer<PC> pc;

struct Uniform {
    row_major float4x4 view;
    row_major float4x4 projection;
};

ConstantBuffer<Uniform> ubo : register(b0, space0);

[[vk::combinedImageSampler]]
Texture2D texture : register(t0, space1);
[[vk::combinedImageSampler]]
SamplerState texSampler : register(s0, space1);

[shader("vertex")]
VertexOutput vertMain(VertexInput vIn) {
    VertexOutput vOut;
    vOut.pos = mul(ubo.projection, (mul(ubo.view, mul(pc.model, float4(vIn.pos, 1.f)))));
    // vOut.pos = float4(vIn.pos, 1.f);
    vOut.color = vIn.color;
    vOut.uv = vIn.uv;
    return vOut;
}

[shader("pixel")]
float4 fragMain(VertexOutput fIn) : SV_Target {
    return texture.Sample(texSampler, fIn.uv);
    // return float4(1.f, 0.f, 0.f, 0.5f);
}

