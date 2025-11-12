#include "12.Shared.hlsli"

//--------------------------------------------------------------------------------------
// Shadow Vertex Shader
//--------------------------------------------------------------------------------------

// 라이트 시점에서 깊이만 찍는다. 

PS_INPUT ShadowVS(VS_SHADOW_INPUT input)
{
    PS_INPUT output = (PS_INPUT) 0;

    float4x4 ModelToWorld;

    if (gIsRigid == 1)
    {
        ModelToWorld = gWorld;
    }
    else
    {
        float4x4 OffsetPose[4];
        OffsetPose[0] = mul(gBoneOffset[input.BoneIndices.x], gBonePose[input.BoneIndices.x]);
        OffsetPose[1] = mul(gBoneOffset[input.BoneIndices.y], gBonePose[input.BoneIndices.y]);
        OffsetPose[2] = mul(gBoneOffset[input.BoneIndices.z], gBonePose[input.BoneIndices.z]);
        OffsetPose[3] = mul(gBoneOffset[input.BoneIndices.w], gBonePose[input.BoneIndices.w]);
        
        float4x4 WeightedOffsetPose;
        WeightedOffsetPose = mul(input.BlendWeights.x, OffsetPose[0]);
        WeightedOffsetPose += mul(input.BlendWeights.y, OffsetPose[1]);
        WeightedOffsetPose += mul(input.BlendWeights.z, OffsetPose[2]);
        WeightedOffsetPose += mul(input.BlendWeights.w, OffsetPose[3]);

        ModelToWorld = mul(WeightedOffsetPose, gWorld); // 포즈변환 + World까지 누적
    }

    float4 worldPos = mul(input.Pos, ModelToWorld);
    float4 lightViewPos = mul(worldPos, mLightView); // 라이트가 보는 시점 
    output.Pos = mul(lightViewPos, mLightProjection);
    
    return output;
} 