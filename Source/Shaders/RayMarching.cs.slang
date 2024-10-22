#include "Core/SampleAppShared.slangh"

import Core.CameraData;
RWTexture2D<float4> dstTex;

uint2 frameDim;
CameraData cameraData;
SampleAppParam params;

struct VolData {
    Texture3D<float> volTex;
    uint3 volDim;

    float3 getNormalizedVolBounds() {
        float3 bounds = 1.f;
        bounds.y = 1.f / volDim.x * volDim.y;
        bounds.z = 1.f / volDim.x * volDim.z;
        return bounds;
    }

    float getVolCell(int3 texLoc) {
        if (any(texLoc > int3(volDim)) || any(texLoc < 0)) {
            return 0.f;
        }
        return volTex[texLoc];
    }

    float getVolData(float3 texLoc) {
        int3 voxelIndex0 = int3(floor(texLoc));

        int3 voxelIndex1 = voxelIndex0 + int3(1, 1, 1);

        float3 frac = texLoc - float3(voxelIndex0);

        float v000 = getVolCell(int3(voxelIndex0.x, voxelIndex0.y, voxelIndex0.z));
        float v100 = getVolCell(int3(voxelIndex1.x, voxelIndex0.y, voxelIndex0.z));
        float v010 = getVolCell(int3(voxelIndex0.x, voxelIndex1.y, voxelIndex0.z));
        float v001 = getVolCell(int3(voxelIndex0.x, voxelIndex0.y, voxelIndex1.z));
        float v101 = getVolCell(int3(voxelIndex1.x, voxelIndex0.y, voxelIndex1.z));
        float v011 = getVolCell(int3(voxelIndex0.x, voxelIndex1.y, voxelIndex1.z));
        float v110 = getVolCell(int3(voxelIndex1.x, voxelIndex1.y, voxelIndex0.z));
        float v111 = getVolCell(int3(voxelIndex1.x, voxelIndex1.y, voxelIndex1.z));

        float c00 = lerp(v000, v100, frac.x);
        float c01 = lerp(v001, v101, frac.x);
        float c10 = lerp(v010, v110, frac.x);
        float c11 = lerp(v011, v111, frac.x);

        float c0 = lerp(c00, c10, frac.y);
        float c1 = lerp(c01, c11, frac.y);

        float value = lerp(c0, c1, frac.z);

        return value;
    }
};

struct ShadingData {
    float3 posW;
    float density;
    float3 normW;
};

struct Ray {
    float3 origin;
    float3 dir;
};

VolData volData;

float3 computeCameraRayPerspective(float2 posScreen, float2 frameDim) {
    float2 p = posScreen / float2(frameDim);
    float2 ndc = float2(2, -2) * p + float2(-1, 1);

    return normalize(ndc.x * cameraData.cameraU + ndc.y * cameraData.cameraV + cameraData.cameraW);
}

void computeCameraRayOrtho(float2 posScreen, float2 frameDim, out Ray ray) {
    float2 p = posScreen / float2(frameDim);
    float2 ndc = float2(2, -2) * p + float2(-1, 1);

    float3 toScene = normalize(cameraData.target - cameraData.posW);

    ray.dir = toScene;
    ray.origin = cameraData.posW + (ndc.x * cameraData.cameraU + ndc.y * cameraData.cameraV) * 0.006;
}

float3 worldPositionToTexCoord(const float3 posW, const float3 boundsHalf) {
    // volume texture: (1.0 x 1.0 x 1.0)
    //         ^ z
    //         |
    //  x<-----
    //          \ 
    //           \ y

    float3 texLoc = posW.xzy;
    texLoc = texLoc + boundsHalf;
    texLoc /= boundsHalf * 2.f;
    return texLoc * float3(volData.volDim);
}

bool rayMarchStep(float3 posW, out float value, out float3 texLoc) {
    float3 bounds = volData.getNormalizedVolBounds();
    float3 boundsHalf = bounds * 0.5;
    if (any(posW < -boundsHalf) || any(posW > boundsHalf))
        return false;
    texLoc = worldPositionToTexCoord(posW, boundsHalf);
    float density = volData.getVolData(texLoc);

    value = density;
    return true;
}

bool transportFunc(float value, out float3 color) {
    if (value < params.filterValue)
        return false;

    color = float3(value / 500.0);
    return true;
}

float3 computeNormal(float3 texLoc) {
    float epsilon = 1.0f / volData.volDim.x;

    float sampleX1 = volData.getVolData(texLoc + float3(epsilon, 0, 0));
    float sampleX2 = volData.getVolData(texLoc - float3(epsilon, 0, 0));
    float sampleY1 = volData.getVolData(texLoc + float3(0, epsilon, 0));
    float sampleY2 = volData.getVolData(texLoc - float3(0, epsilon, 0));
    float sampleZ1 = volData.getVolData(texLoc + float3(0, 0, epsilon));
    float sampleZ2 = volData.getVolData(texLoc - float3(0, 0, epsilon));

    float3 gradient;
    gradient.x = (sampleX1 - sampleX2) * 0.5f;
    gradient.y = (sampleY1 - sampleY2) * 0.5f;
    gradient.z = (sampleZ1 - sampleZ2) * 0.5f;

    float3 normal = -normalize(gradient);

    return normal;
}

bool rayMarch(const Ray ray, out ShadingData sd) {
    float stepSize = 0.01;
    const int steps = 1000;
    float3 p = ray.origin;
    float3 accumulateColor = float3(0.0);
    float3 stepColor = float3(0.0);
    float3 texLoc;
    float stepValue;

    uint hitCount = 0;
    const uint kMaxHitCount = 5;

    for (int i = 0; i < steps; i++) {
        if (rayMarchStep(p, stepValue, texLoc)) {
            // bisec ray marching
            if (transportFunc(stepValue, stepColor)) {
                p -= stepSize * ray.dir;
                stepSize *= 0.5f;
                hitCount++;
                if (hitCount >= kMaxHitCount) {
                    accumulateColor = stepColor;
                    sd.density = stepValue;
                    sd.posW = p;
                    sd.normW = computeNormal(texLoc);
                    return true;
                }
            }
        }
        p += stepSize * ray.dir;
    }
    return false;
}

float3 phongShade(const ShadingData sd) {
    float3 toLight = cameraData.posW - sd.posW;
    float3 L = normalize(toLight);

    float3 ambient = 0.2f;
    float3 diffuse = 0.4f * 1.f * max(0, dot(sd.normW, L));
    float3 specular = 0.3f * 1.f * max(0, dot(reflect(-L, sd.normW), L));

    return ambient + diffuse + specular;
}

void execute(uint2 pixel) {
    Ray ray;
    computeCameraRayOrtho(pixel + 0.5, frameDim, ray);

    ShadingData sd;
    if (rayMarch(ray, sd)) {
        float3 shadingColor;
        switch (params.shadingMode) {
        case ShadingMode::FlatShade:
            shadingColor = phongShade(sd);
            break;
        case ShadingMode::Normal:
            shadingColor = sd.normW * 0.5 + 0.5;
            break;
        case ShadingMode::TransportFunc:
            // TODO
            shadingColor = 1.f;
            break;
        }
        dstTex[pixel] = float4(shadingColor, 1.0f);
    } else {
        dstTex[pixel] = float4(0.3);
    }
}

[shader("compute")]
[numthreads(16, 16, 1)]
void main(uint3 threadId: SV_DispatchThreadID) {
    uint2 pixel = threadId.xy;
    if (any(pixel >= frameDim.xy))
        return;
    execute(pixel);
}
