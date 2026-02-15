struct VSInput {
    float2 pos;
    float2 uv;
    float4 color;
};

struct VSOutput {
    float4 pos : SV_POSITION;
    float4 color;
    float2 uv;
};

struct PC {
    float2 uScale;
    float2 uTranslate;
};

[[vk_push_constant]] ConstantBuffer<PC> pc;

[shader("vertex")]
VSOutput vsMain(VSInput vIn) {
    VSOutput output;
    output.pos = float4(pc.uScale * vIn.pos + pc.uTranslate, 0.0f, 1.0f);
    output.color = vIn.color;
    output.uv = vIn.uv;
    return output;
}

[[vk_binding(0, 0)]]
Sampler2D sampledTexture;

float3 sRGBtoLinear(float3 srgb) {
    return pow(srgb, 2.2f);
}

[shader("fragment")]
float4 fsMain(VSOutput fIn) : SV_Target {
    float4 color = fIn.color * sampledTexture.Sample(fIn.uv);
    return float4(sRGBtoLinear(color.rgb), color.w);
}