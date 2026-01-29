struct VertexInput {
    float3 pos : POSITION0;
    float4 color : COLOR1;
    float2 uv : TEXCOORD2;
};

struct VertexOutput {
    float4 pos : SV_Position;
    float4 color : COLOR0;
    float2 uv : TEXCOORD1;
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

Texture2D t_texture : register(t0, space1);
SamplerState s_sampler : register(s0, space1);

[shader("vertex")]
VertexOutput vertMain(VertexInput vIn) {
    VertexOutput vOut;
    // vOut.pos = mul(ubo.projection, (mul(ubo.view, mul(pc.model, float4(vIn.pos, 1.f)))));
    vOut.pos = float4(vIn.pos, 1.f);
    vOut.color = vIn.color;
    vOut.uv = vIn.uv;
    return vOut;
}

[shader("pixel")]
float4 fragMain(VertexOutput fIn) : SV_Target {
    return t_texture.Sample(s_sampler, fIn.uv);
    // return float4(1.f, 0.f, 0.f, 0.5f);
}


