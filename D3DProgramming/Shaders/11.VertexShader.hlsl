#include "11.Shared.hlsli"

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT) 0;

    float4x4 ModelToWorld;

    if (gIsRigid > 0.5f)
    {
        ModelToWorld = gWorld;
    }
    else
    {
         // v * OffsetMatrix * PoseMatrix -> v * AB(먼저계산)
        float4x4 OffsetPose[4];
        OffsetPose[0] = mul(gBoneOffset[input.BoneIndices.x], gBonePose[input.BoneIndices.x]);
        OffsetPose[1] = mul(gBoneOffset[input.BoneIndices.y], gBonePose[input.BoneIndices.y]);
        OffsetPose[2] = mul(gBoneOffset[input.BoneIndices.z], gBonePose[input.BoneIndices.z]);
        OffsetPose[3] = mul(gBoneOffset[input.BoneIndices.w], gBonePose[input.BoneIndices.w]);

        // 4개를 가중치 누적합으로 하나로 변경
        float4x4 WeightedOffsetPose;
        WeightedOffsetPose = mul(input.BlendWeights.x, OffsetPose[0]);
        WeightedOffsetPose += mul(input.BlendWeights.y, OffsetPose[1]);
        WeightedOffsetPose += mul(input.BlendWeights.z, OffsetPose[2]);
        WeightedOffsetPose += mul(input.BlendWeights.w, OffsetPose[3]);

        ModelToWorld = mul(WeightedOffsetPose, gWorld); // 포즈변환 + World까지 누적
    }

    float4 worldPos = mul(input.Pos, ModelToWorld);
    output.WorldPos = worldPos.xyz;
    output.Pos = mul(mul(worldPos, gView), gProjection);

    float3x3 ModelToWorld3x3 = (float3x3) ModelToWorld;
    output.Norm = normalize(mul(input.Norm, ModelToWorld3x3));
    output.Tangent = normalize(mul(input.Tangent, ModelToWorld3x3));
    output.Binormal = normalize(mul(input.Binormal, ModelToWorld3x3));

    output.Tex = input.Tex;
    
    return output;
}