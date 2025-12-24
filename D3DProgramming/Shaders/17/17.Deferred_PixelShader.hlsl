#include "17.Shared.hlsli"

//--------------------------------------------------------------------------------------
// Deferred Directional Lighting Pixel Shader
//--------------------------------------------------------------------------------------

// 상수 값 
static const float PI = 3.14159265; 
static const float Epsilon = 0.00001; 

// <F> [ Fresnel효과의 Schlick 근사 (RGB 반환) ]
float3 fresnelSchlick(float3 F0, float cosTheta)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

// <D> [ NDF (Normal Distribution Function) - GGX ]
float ndfGGX(float cosNH, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    
    float denom = (cosNH * cosNH) * (a2 - 1.0) + 1.0;
    return a2 / max(PI * denom * denom, Epsilon);
}

// <G> [ Schlick-GGX Geometry Term (1방향) ]
float G_Schlick_GGX(float cosX, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0f; // (r^2)/8 : Epic Games 에서 만든 근사값  
    return cosX / max(cosX * (1.0 - k) + k, Epsilon);
}

// <G> [ Smith Geometry (양방향) ]
float G_Smith(float cosNL, float cosNV, float roughness)
{
    return G_Schlick_GGX(cosNL, roughness) * G_Schlick_GGX(cosNV, roughness);
}


float4 main(VS_OUTPUT_FULL input) : SV_Target
{
    float2 uv = input.UV;

    // ------------------------------
    // G-Buffer Decode
    // ------------------------------
    float3 worldPos = txPosition_G.Sample(samLinear, uv).xyz;
    if (length(worldPos) < 1e-5)
        discard; // SkyBox 픽셀 버림
    
    float3 N = normalize(txNormal_G.Sample(samLinear, uv).xyz);
    float3 albedo = txAlbedo_G.Sample(samLinear, uv).rgb;
    float2 mr = txMR_G.Sample(samLinear, uv).rg;
    float3 emissive = txEmissive_G.Sample(samLinear, uv).rgb;
    
    float metallic = mr.x;
    float roughness = max(0.01, mr.y);
    
    
    // ----------------------------------------------------------------------
    // View / Light / Half 벡터 계산 
    // ----------------------------------------------------------------------
    float3 V = normalize(vEyePos.xyz - worldPos);   // 시선 방향
    float3 L = normalize(-vLightDir.xyz);           // 광원 방향
    float3 H = normalize(V + L);                    // Half 벡터

    float cosNL = saturate(dot(N, L));
    float cosNV = saturate(dot(N, V));
    float cosNH = saturate(dot(N, H));
    float cosVH = saturate(dot(V, H));
    float cosLH = saturate(dot(L, H));
    
    // ------------------------------
    // Fresnel / BRDF
    // ------------------------------
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo, metallic);

    float3 F = fresnelSchlick(F0, cosVH);
    float D = ndfGGX(cosNH, roughness);
    float G = G_Smith(cosNL, cosNV, roughness);

    float3 kd = lerp(1.0 - F, 0.0, metallic);
    float3 diffuseBRDF = kd * albedo / PI;
    float3 specularBRDF = (F * D * G) / max(Epsilon, 4.0 * cosNL * cosNV);

    
    
    // ----------------------------------------------------------------------
    // 7. Shadow : PCF (WorldPos → Light VP)
    // ----------------------------------------------------------------------
    // PositionShadow 는 Light View Projection 좌표(NDC) 기반 -> 직접 변환해야 함    
    float shadowFactor = 1.0f;
    
    float4 shadowPos = mul(float4(worldPos, 1.0f), mLightView);
    shadowPos = mul(shadowPos, mLightProjection);
    
    float currentDepth = shadowPos.z / shadowPos.w;
    
    // NDC -> Texture UV 변환
    float2 shadowUV = shadowPos.xy / shadowPos.w * 0.5 + 0.5;
    shadowUV.y = 1.0 - shadowUV.y; // DX 기준 Y 반전
    
    if (shadowUV.x >= 0.0f && shadowUV.x <= 1.0f && shadowUV.y >= 0.0f && shadowUV.y <= 1.0f) // 그림자 맵 범위 내부인지 체크
    {
        float texelSize = 1.0f / 8192.0f; // ShadowMapSize = 8192.0; // ShadowMap 해상도
        const float depthBias = 0.0005f;
        float sum = 0.0f;
        
        // 3x3 PCF
        [unroll]
        for (int y = -1; y <= 1; ++y)
        {
            [unroll]
            for (int x = -1; x <= 1; ++x)
            {
                float2 sampleUV = shadowUV + float2(x, y) * texelSize;
                sum += txShadowMap.SampleCmpLevelZero(samShadow, sampleUV, currentDepth - depthBias);
            }
        }
        shadowFactor = sum / 9.0f; // 평균값
    }
    
    
    // ----------------------------------------------------------------------
    // 8. Direct lighting 출력 조명 계산
    // ----------------------------------------------------------------------
    float lightIntensity = vLightColor.a;
    float3 lightRadiance = vLightColor.rgb * lightIntensity;
    
    float3 DirectLight = (diffuseBRDF + specularBRDF) * lightRadiance * cosNL * shadowFactor;
    
    
    
    // ----------------------------------------------------------------------
    // 9. IBL : (Diffuse + Specular) split-sum approximation 
    // ----------------------------------------------------------------------
    
    float3 IndirectLight_IBL = float3(0.0f, 0.0f, 0.0f);

    if (useIBL == 1)
    {
        // [ Diffuse IBL ] (Irradiance) -----------------------------------------
        float3 irradiance = txIBL_Diffuse.Sample(samLinearIBL, N).rgb;
    
        float3 F_IBL = fresnelSchlick(F0, cosVH);
        float3 kd_IBL = lerp(1.0 - F_IBL, 0.0, metallic);
        float3 diffuseIBL = kd_IBL * albedo / PI * irradiance;

    
        // [ Specular IBL ] (Prefiltered env + BRDF LUT) -------------------------
        float3 R = 2.0 * cosNV * N - V; 
    
        uint specularTextureLevels, width, height;
        txIBL_Specular.GetDimensions(0, width, height, specularTextureLevels); //  텍스쳐의 최대 LOD 개수
          
        float3 PrefilteredColor = txIBL_Specular.SampleLevel(samLinearIBL, R, roughness * (specularTextureLevels-1)).rgb;
        
        float2 brdf = txIBL_BRDF_LUT.Sample(samClampIBL, float2(cosVH, roughness)).rg;
        
        float3 specularIBL = PrefilteredColor * (F0 * brdf.x + brdf.y);
    
        IndirectLight_IBL = diffuseIBL + specularIBL;
    }
    
    
    // ----------------------------------------------------------------------
    // 10. 최종 조명  
    // ----------------------------------------------------------------------
    float3 colorLinear = DirectLight + IndirectLight_IBL + emissive;
    
    // Deferred는 항상 불투명함 
    float4 finalColor = float4(colorLinear, 1.0f);
    return finalColor;
}