#include "11.ToonShared.hlsli"

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
VS_OUTLINE_OUTPUT OutlineVS(VS_INPUT input)
{
    VS_OUTLINE_OUTPUT output = (VS_OUTLINE_OUTPUT) 0;

    float4x4 ModelToWorld;

    if (gIsRigid > 0.5f)
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

        ModelToWorld = mul(WeightedOffsetPose, gWorld);
    }

    float3x3 ModelToWorld3x3 = (float3x3) ModelToWorld;
    float3 worldNormal = normalize(mul(input.Norm, ModelToWorld3x3));

    float4 worldPos = mul(input.Pos, ModelToWorld);
    worldPos.xyz += worldNormal * OutlineThickness;

    output.Pos = mul(mul(worldPos, gView), gProjection);
    output.Color = OutlineColor;
    return output;
}