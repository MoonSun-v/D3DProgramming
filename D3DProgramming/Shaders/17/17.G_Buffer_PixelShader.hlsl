#include "17.Shared.hlsli"

//--------------------------------------------------------------------------------------
// G-Buffer Pixel Shader
//--------------------------------------------------------------------------------------

PS_OUTPUT_GBUFFER main(PS_INPUT_GBUFFER input)
{
    PS_OUTPUT_GBUFFER o;

    // ------------------------------
    // Texture Sampling
    // ------------------------------
    float4 baseColorTex = txBaseColor.Sample(samLinear, input.Tex);
    float4 normalTex = txNormal.Sample(samLinear, input.Tex);
    float metalTex = txMetallic.Sample(samLinear, input.Tex).r;
    float roughTex = txRoughness.Sample(samLinear, input.Tex).r;
    // float4 opacityTex = txOpacity.Sample(samLinear, input.Tex);
    float3 emissive = txEmissive.Sample(samLinear, input.Tex).rgb;
    
    // ------------------------------
    // Opacity Clip
    // ------------------------------
    // clip(opacityTex.a - 0.5f);

    // ------------------------------
    // Normal Mapping
    // ------------------------------
    float3 N = normalize(input.Normal);

    if (useTexture_Normal == 1)
    {
        float3 T = normalize(input.Tangent);
        float3 B = normalize(input.Binormal);

        float3x3 TBN = float3x3(T, B, N);
        float3 normalMap = DecodeNormal(normalTex.rgb);
        N = normalize(mul(normalMap, TBN));
    }

    // ------------------------------
    // Material Parameters
    // ------------------------------
    float3 albedo = useTexture_BaseColor
        ? SRGBToLinear(baseColorTex.rgb)
        : SRGBToLinear(manualBaseColor.rgb);

    float metallic = useTexture_Metallic
        ? saturate(metalTex * gMetallicMultiplier.x)
        : saturate(gMetallicMultiplier.x);

    float roughness = useTexture_Roughness
        ? saturate(roughTex * gRoughnessMultiplier.x)
        : saturate(gRoughnessMultiplier.x);

    roughness = max(0.01, roughness);
    

    // ------------------------------
    // G-Buffer Output
    // ------------------------------
    o.WorldPos  = float4(input.WorldPos, 1.0f);
    o.Normal    = float4(normalize(N), 1.0f);
    o.Albedo    = float4(albedo, 1.0f);
    o.MR        = float4(metallic, roughness, 0 , 0);
    o.Emissive  = float4(emissive, 1);

    return o;
}