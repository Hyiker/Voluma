import Core.CameraData;
RWTexture2D<float4> dstTex;

uint2 frameDim;
CameraData cameraData;

StructuredBuffer<float> volBuffer;

float3 computeCameraRayDirection(float2 posScreen, float2 frameDim) {
    float2 p = posScreen / float2(frameDim);
    float2 ndc = float2(2, -2) * p + float2(-1, 1);

    return normalize(ndc.x * cameraData.cameraU + ndc.y * cameraData.cameraV + cameraData.cameraW);
}

int worldPosToBufferIndex(float3 posW) {
    if (any(posW > 1.f) || any(posW < -1.f))
        return -1;
    float3 nPos = posW * 0.5f + 0.5f;
    const int kBufferSize = 631 * 584 * 584;
    float3 posO = float3(nPos.x, nPos.y, nPos.z);
    int index = posO.y * 631 * 584 * 584 + posO.x * 584 * 584 + posO.z * 584;
    return index;
}

bool rayMarchStep(float3 posW, out float3 color) {
    int index = worldPosToBufferIndex(posW);
    if (index < 0 || index >= 584 * 584 * 631) {
        return false;
    }
    float density = volBuffer[index];
    if (density <= 100.f) {
        return false;
    }
    color = float3(density / 3000.f, 0.0, 0.0);
    return true;
}

float3 rayMarch(float3 direction) {
    float3 origin = cameraData.posW;

    float t = 0.01;
    const int steps = 1000;
    float3 p = origin;
    float3 stepColor = float3(0.0);
    for (int i = 0; i < steps; i++) {
        if (rayMarchStep(p, stepColor)) {
            return stepColor;
        }
        p += t * direction;
    }
    return float3(0.0);
}

[shader("compute")]
[numthreads(16, 16, 1)]
void main(uint3 threadId: SV_DispatchThreadID) {
    uint2 pixel = threadId.xy;
    if (any(pixel >= frameDim.xy))
        return;

    dstTex[pixel] = float4(rayMarch(computeCameraRayDirection(pixel + 0.5, frameDim)), 1.0f);
}
