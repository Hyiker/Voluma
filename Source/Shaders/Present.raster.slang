
Texture2D<float4> srcTex;

// Uniform data to be passed from application -> shader.
cbuffer Uniforms {
    float4x4 modelViewProjection;
}

[shader("vertex")]
float4 vertexMain(float2 position: POSITION) : SV_Position {
    return float4(position, 0.5, 1.0);
}

[shader("fragment")]
float4 fragmentMain(float4 sv_position: SV_Position) : SV_Target {
    uint2 pixel = uint2(sv_position.xy);
    return srcTex[pixel];
}
