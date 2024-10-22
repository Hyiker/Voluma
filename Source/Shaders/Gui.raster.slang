SamplerState gSampler;
Texture2D<float4> gFont;

float2 scale;
float2 offset;

struct VsIn {
    float2 pos : POSITION;
    float2 texC : TEXCOORD0;
    float4 color : COLOR;
};

struct VsOut {
    float4 color : COLOR;
    float2 texC : TEXCOORD0;
    float4 pos : SV_POSITION;
};

[shader("vertex")]
VsOut vsMain(VsIn vIn) {
    VsOut vOut;
    vOut.color = vIn.color;
    vOut.texC = vIn.texC;
    vOut.pos.xy = vIn.pos.xy * scale + offset;
    vOut.pos.zw = float2(0, 1);
    return vOut;
}

[shader("fragment")]
float4 psMain(VsOut vOut) : SV_TARGET {
    float4 fontMask = gFont.Sample(gSampler, vOut.texC);
    float4 c = vOut.color * fontMask;
    return c;
}
