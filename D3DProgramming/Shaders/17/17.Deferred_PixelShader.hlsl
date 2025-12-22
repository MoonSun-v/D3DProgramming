#include "17.Shared.hlsli"

//--------------------------------------------------------------------------------------
// Deferred Directional Lighting Pixel Shader
//--------------------------------------------------------------------------------------

// 상수 값 
static const float PI = 3.14159265; // 원주율
static const float Epsilon = 0.00001; // 아주 작은 값 


// <F> [ Fresnel효과의 Schlick 근사 (RGB 반환) ]
// - F0: 정면에서 보는 반사율(금속/비금속에 따라 다른 값)
// - cosTheta: 빛이 표면에 얼마나 수직으로 들어오는가?
float3 fresnelSchlick(float3 F0, float cosTheta)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}


// <D> [ NDF (Normal Distribution Function) - GGX ]
// - cosNH : normal과 half-vector 사이의 각도
// - roughness : 표면 거칠기(0 = 매끈, 1 = 거침)
float ndfGGX(float cosNH, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    
    float denom = (cosNH * cosNH) * (a2 - 1.0) + 1.0;
    return a2 / max(PI * denom * denom, Epsilon);
}


// <G> [ Schlick-GGX Geometry Term (1방향) ]
// 표면의 미세한 요철 때문에 빛이 일부 가려지는 효과
float G_Schlick_GGX(float cosX, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0f; // * 0.125; // (r^2)/8 : Epic Games 에서 만든 근사값  
    return cosX / max(cosX * (1.0 - k) + k, Epsilon);
}

// <G> [ Smith Geometry (양방향) ] : 두 방향의 Geometry Term 곱
float G_Smith(float cosNL, float cosNV, float roughness)
{
    return G_Schlick_GGX(cosNL, roughness) * G_Schlick_GGX(cosNV, roughness);
}


float4 main(VS_OUT_FS input) : SV_Target
{
    float2 uv = input.uv;

    // ------------------------------
    // G-Buffer Decode
    // ------------------------------
    float3 worldPos = txPosition_G.Sample(samLinear, uv).xyz;
    float3 N = normalize(txNormal_G.Sample(samLinear, uv).xyz);
    float3 albedo = txAlbedo_G.Sample(samLinear, uv).rgb;
    float2 mr = txMR_G.Sample(samLinear, uv).rg;
    float3 emissive = txEmissive_G.Sample(samLinear, uv).rgb;
    
    float metallic = mr.x;
    float roughness = max(0.01, mr.y);
    
    
    // ----------------------------------------------------------------------
    // View / Light / Half 벡터 계산 
    // ----------------------------------------------------------------------
    float3 V = normalize(vEyePos.xyz - worldPos); // 시선 방향
    float3 L = normalize(-vLightDir.xyz); // 광원 방향
    float3 H = normalize(V + L); // Half 벡터

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
    //float shadowFactor = 1.0f;
    //float currentShadowDepth = input.PositionShadow.z / input.PositionShadow.w;
    
    //// NDC -> Texture 좌표계 변환
    //float2 ShadowUV = input.PositionShadow.xy / input.PositionShadow.w;
    //ShadowUV.y = -ShadowUV.y;
    //ShadowUV = ShadowUV * 0.5f + 0.5f;
    
    float shadowFactor = 1.0f;

    float4 shadowPos = mul(float4(worldPos, 1.0f), mWorld);
    shadowPos = mul(shadowPos, mLightView);
    // float4 shadowPos = mul(float4(worldPos, 1.0f), mLightView);
    shadowPos = mul(shadowPos, mLightProjection);

    float currentDepth = shadowPos.z / shadowPos.w;

    float2 shadowUV = shadowPos.xy / shadowPos.w;
    shadowUV.y = -shadowUV.y;
    shadowUV = shadowUV * 0.5 + 0.5;
    
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
                sum += txShadowMap.SampleCmpLevelZero(samShadow, sampleUV, currentDepth /*currentShadowDepth*/ - depthBias);
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
        float3 R = 2.0 * cosNV * N - V; // float3 R = reflect(-V, N);
    
        uint specularTextureLevels, width, height;
        txIBL_Specular.GetDimensions(0, width, height, specularTextureLevels); //  텍스쳐의 최대 LOD 개수를 구한다.	
        
        // Lr( View,Normal의 반사벡터) 와 거칠기를 사용하여 반사 빛을 샘플링한다. 
        // 거칠기에 따라 뭉게진 반사 빛을 표현하기위해  LOD 선형보간이 적용된다.    
        float3 PrefilteredColor = txIBL_Specular.SampleLevel(samLinearIBL, R, roughness * (specularTextureLevels-1)).rgb;

        // dot(Normal,View) , roughness를 텍셀좌표로 미리계산된 F*G , G 평균값을 샘플링한다  
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