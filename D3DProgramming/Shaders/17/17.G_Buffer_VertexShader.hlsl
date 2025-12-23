#include "17.Shared.hlsli"

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
PS_INPUT_GBUFFER main(VS_INPUT input)
{
    PS_INPUT_GBUFFER output = (PS_INPUT_GBUFFER) 0;

    float4x4 ModelToWorld;

    // ----------------------------
    // Skinning / Rigid ºÐ±â
    // ----------------------------
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

        float4x4 WeightedOffsetPose = 0;
        WeightedOffsetPose = mul(input.BlendWeights.x, OffsetPose[0]);
        WeightedOffsetPose += mul(input.BlendWeights.y, OffsetPose[1]);
        WeightedOffsetPose += mul(input.BlendWeights.z, OffsetPose[2]);
        WeightedOffsetPose += mul(input.BlendWeights.w, OffsetPose[3]);

        ModelToWorld = mul(WeightedOffsetPose, gWorld);
    }

    // ----------------------------
    // Position
    // ----------------------------
    float4 worldPos = mul(float4(input.Pos, 1.0f), ModelToWorld);
    output.WorldPos = worldPos.xyz;
    output.Pos = mul(mul(worldPos, gView), gProjection);

    // ----------------------------
    // Normal / TBN
    // ----------------------------
    float3x3 M = (float3x3) ModelToWorld;

    output.Normal = normalize(mul(input.Norm, M));
    output.Tangent = normalize(mul(input.Tangent, M));
    output.Binormal = normalize(mul(input.Binormal, M));

    output.Tex = input.Tex;

    return output;
}
