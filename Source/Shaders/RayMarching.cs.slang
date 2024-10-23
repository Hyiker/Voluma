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

    float3 computeGradient(float3 texLoc) {
        float epsilon = 1.0f / volData.volDim.x * 0.1f;

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

        return gradient;
    }
    float3 computeNormal(float3 texLoc) { return -normalize(computeGradient(texLoc)); }

    float3 worldPositionToTexCoord(const float3 posW) {
        // volume texture: (1.0 x 1.0 x 1.0)
        //         ^ z
        //         |
        //  x<-----
        //          \ 
        //           \ y

        float3 bounds = volData.getNormalizedVolBounds();
        float3 boundsHalf = bounds * 0.5;

        float3 texLoc = posW.xzy;
        texLoc = texLoc + boundsHalf;
        texLoc /= boundsHalf * 2.f;
        return texLoc * float3(volData.volDim);
    }

    bool isInside(float3 posW) {
        float3 bounds = volData.getNormalizedVolBounds();
        float3 boundsHalf = bounds * 0.5;
        if (any(posW < -boundsHalf) || any(posW > boundsHalf))
            return false;
        return true;
    }

    bool rayBoxIntersection(float3 rayOrigin, float3 rayDir, out float2 t) {
        float3 boxSize = getNormalizedVolBounds();
        float3 boxMin = -boxSize * 0.5f;
        float3 boxMax = boxSize * 0.5f;

        float3 invDir = 1.0f / rayDir;
        float3 tMin = (boxMin - rayOrigin) * invDir;
        float3 tMax = (boxMax - rayOrigin) * invDir;

        float3 t0 = min(tMin, tMax);
        float3 t1 = max(tMin, tMax);

        float tNear = max(max(t0.x, t0.y), t0.z);
        float tFar = min(min(t1.x, t1.y), t1.z);
        if (tNear > tFar || tFar < 0.0f) {
            return false;
        }

        // 返回最近和最远交点的参数t
        t = float2(tNear, tFar);
        return true;
    }
};

struct ShadingData {
    float3 posW;
    float density;
    float3 normW;
    float4 transportColor;
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

bool rayMarchStep(float3 posW, out float value, out float3 texLoc) {
    if (!volData.isInside(posW))
        return false;
    texLoc = volData.worldPositionToTexCoord(posW);
    float density = volData.getVolData(texLoc);

    value = density;
    return true;
}

struct TransportStep {
    float4 color;
    int valueThreshold;
};

float4 transportFunc(float value, inout int nextTFIndex) {
    const int kBreakpointCount = 3;
    const TransportStep kTransportSteps[kBreakpointCount] = {
        { float4(0.1, 0.1, 0.7, 0.2), 500 }, { float4(0.2, 0.2, 0.4, 0.3), 1000 }, { float4(1.0, 1.0, 1.0, 0.5), 1800 }
    };
    if (nextTFIndex >= kBreakpointCount) {
        return 0.f;
    }
    TransportStep nextStep = kTransportSteps[nextTFIndex];
    if (value >= nextStep.valueThreshold) {
        nextTFIndex++;
        return nextStep.color;
    }
    return 0.f;
}

float3 phongShading(const ShadingData sd, const float3 diffuseColor = 0.4f) {
    float3 toLight = float3(0.0, 0.0, -3.0) - cameraData.target;
    float3 L = normalize(toLight);

    float3 ambient = 0.2f;
    float3 diffuse = diffuseColor * 1.f * max(0, dot(sd.normW, L));
    // Remove specular here to make it look smoother
    // float3 specular = 0.5f * 1.f * pow(max(0, dot(reflect(-L, sd.normW), L)), 5.f);

    return ambient + diffuse;
}

bool rayMarch(const Ray ray, out ShadingData sd) {
    const int kMaxSteps = 1000;
    const float kInitialStep = 1.0f / kMaxSteps;
    float stepSize = kInitialStep;

    float3 stepColor = float3(0.0);
    float3 texLoc;
    float stepValue;

    float2 t;
    if (!volData.rayBoxIntersection(ray.origin, ray.dir, t)) {
        return false;
    }
    float3 p = ray.origin + ray.dir * t.x;

    int nextTFIndex = 0;
    sd.transportColor = 0.f;

    for (int i = 0; i < kMaxSteps; i++) {
        if (rayMarchStep(p, stepValue, texLoc) && stepValue >= params.filterValue) {
            sd.density = stepValue;
            sd.posW = p;
            sd.normW = volData.computeNormal(texLoc);
            if (params.shadingMode == ShadingMode::TransportFunc) {
                float4 c = transportFunc(stepValue, nextTFIndex);
                ;
                sd.transportColor += float4(c.xyz * c.a, c.a);
                if (nextTFIndex == 3) {
                    return true;
                }
            } else {
                return true;
            }
        }
        p += stepSize * ray.dir;
    }
    if (params.shadingMode != ShadingMode::TransportFunc)
        return false;
    return sd.transportColor.a != 0.f;
}

void execute(uint2 pixel) {
    const float3 kBackgroundColor = float3(0.03, 0.3, 0.3);
    Ray ray;
    float2 uv = float2(pixel) / frameDim.xy;
    computeCameraRayOrtho(pixel + 0.5, frameDim, ray);

    ShadingData sd;
    if (rayMarch(ray, sd)) {
        float3 shadingColor;
        switch (params.shadingMode) {
        case ShadingMode::FlatShade:
            shadingColor = phongShading(sd);
            break;
        case ShadingMode::Normal:
            shadingColor = sd.normW * 0.5 + 0.5;
            break;
        case ShadingMode::TransportFunc:
            shadingColor = sd.transportColor.rgb + (1.f - sd.transportColor.a) * kBackgroundColor;
            break;
        }
        dstTex[pixel] = float4(shadingColor, 1.0f);
    } else {
        dstTex[pixel] = float4(kBackgroundColor, 1.f);
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
