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

[[vk_push_constant]]
ConstantBuffer<PC> pc;

[shader("vertex")]
VSOutput vsMain(VSInput vIn) {
    VSOutput output;
    output.pos = float4(pc.uScale * vIn.pos + pc.uTranslate, 0.0f, 1.0f);
    output.color = vIn.color;
    output.uv = vIn.uv;
    return output;
}

// set = 0, binding = 0
[[vk::binding(0, 0)]]
Texture2D<float4> _Texture;

// set = 1, binding = 0
[[vk::binding(0, 1)]]
SamplerState _Sampler;

float3 sRGBtoLinear(float3 srgb) {
    return pow(srgb, 2.2f);
}

[shader("fragment")]
float4 fsMain(VSOutput fIn) : SV_Target {
    float4 color = fIn.color * _Texture.Sample(_Sampler, fIn.uv);
    return float4(sRGBtoLinear(color.rgb), color.w);
}