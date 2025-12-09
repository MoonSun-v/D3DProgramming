#include "15.Shared.hlsli"

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------

// 인코딩/디코딩 
float3 EncodeNormal(float3 N) { return N * 0.5 + 0.5; }
float3 DecodeNormal(float3 N) { return N * 2.0 - 1.0; }


// 상수 값 
static const float PI = 3.14159265;     // 원주율
static const float InvPI = 1.0 / PI;    // 원주율의 역수 
static const float Epsilon = 1e-5;      // 아주 작은 값 


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
    float k = (r * r) * 0.125; // (r^2)/8 : Epic Games 에서 만든 근사값  
    return cosX / max(cosX * (1.0 - k) + k, Epsilon);
}

// <G> [ Smith Geometry (양방향) ] : 두 방향의 Geometry Term 곱
float G_Smith(float cosNL, float cosNV, float roughness)
{
    return G_Schlick_GGX(cosNL, roughness) * G_Schlick_GGX(cosNV, roughness);
}

// sRGB -> Linear 
float3 SRGBToLinear(float3 c) { return pow(c, 2.2f); }

// Linear -> sRGB 
float3 LinearToSRGB(float3 c) { return pow(c, 1.0f / 2.2f); }


// -------------------------------------------------------------------------------
// PBR 최종 공식 => ( DiffuseBRDF + SpecularBRDF ) * 빛의 강도(Radiance) * N·L
// -------------------------------------------------------------------------------
// 
// [ Specular BRDF ] (F·D·G) / (4(N·L)(N·H))
// 
// - F(v,h)     Fresnel 현상 -> 금속성(Metalness) + 입사각 고려
// - D(h)       Normal Distribution Function -> 미세면 거칠기(Roughness)
// - G(l,v,h)   Geometry Shadowing -> 미세면 일부가 가려지거나 반사되지 않는 효과
//
// [ Diffuse BRDF ] kD * BaseColor(Albedo) / π
//  

float4 main(PS_INPUT input) : SV_Target
{
    // 1. 텍스처 샘플링 : C++에서 없으면 기본 텍스쳐 들어가도록 했음 
    float4 baseColorTex = txBaseColor.Sample(samLinear, input.Tex);
    float4 normalTex = txNormal.Sample(samLinear, input.Tex);
    float metalTex = txMetallic.Sample(samLinear, input.Tex).r;
    float roughTex = txRoughness.Sample(samLinear, input.Tex).r;
    float4 emissiveTex = txEmissive.Sample(samLinear, input.Tex);
    float4 opacityTex = txOpacity.Sample(samLinear, input.Tex);

    
    // 2. Opacity Clipping (투명도 테스트) -> DepthBuffer, 알파 블렌딩 필요 없음 
    // 투명도 값이 0.5보다 작으면 픽셀을 그리지 않고 버림
    clip(opacityTex.a - 0.5f);

    
    // 3. Normal Map (TBN)
    float3 N = normalize(input.Norm);
    float3 T = normalize(input.Tangent);
    float3 B = normalize(input.Binormal);

    float3x3 TBN = float3x3(T, B, N);
    
    // 조건에 따라 Normal 결정
    float3 normalMap = DecodeNormal(normalTex.rgb);
    if (useTexture_Normal == 1)
    {
        N = normalize(mul(normalMap, TBN));
    }
    else
    {
        N = normalize(N); // 기본 Normal 유지 (또는 임의 벡터 지정 가능)
    }

    
    // 4. view / light / half 벡터 계산 
    float3 V = normalize(vEyePos.xyz - input.WorldPos); // 시선 방향
    float3 L = normalize(-vLightDir.xyz);               // 광원 방향
    float3 H = normalize(V + L);                        // Half 벡터

    float cosNL = saturate(dot(N, L));
    float cosNV = saturate(dot(N, V));
    float cosNH = saturate(dot(N, H));
    float cosVH = saturate(dot(V, H)); // used for Fresnel
    
    
    // 5. 머티리얼 파라미터 
    // - 텍스처 사용 시 : 텍스처 * multiplier
    // - 텍스처 미사용 시 : GUI값(또는 multiplier 값) 그대로 사용
    float3 albedo = useTexture_BaseColor == 1 ? SRGBToLinear(baseColorTex.rgb) : SRGBToLinear(manualBaseColor.rgb);
    float metallic = useTexture_Metallic == 1 ? saturate(metalTex * gMetallicMultiplier.x) : saturate(gMetallicMultiplier.x); 
    float roughness = useTexture_Roughness == 1 ? saturate(roughTex * gRoughnessMultiplier.x) : saturate(gRoughnessMultiplier.x); 
    roughness = max(0.15, roughness); // 거칠기(roughness)는 최소값 유지 (roughness가 0에 가까우면 수학적 특이점 생김)
    
    // 6. F0  (비금속은 0.04, 금속은 Albedo 사용)
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);
    
    
    // 7. PBR 구성 요소 계산
    float3 F = fresnelSchlick(F0, cosVH);   // Fresnel (Schlick 근사)
    float D = ndfGGX(cosNH, roughness);     // NDF
    float G = G_Smith(cosNL, cosNV, roughness); // Geometry

    // kd:  금속성, F 항에 따른 확산 반사량 감소  kd = (1 - metallic) * (1 - F)
    float3 kd = (1.0 - metallic) * (1.0 - F);

    // diffuse BRDF ( Lambertian Diffuse : 에너지 보존 반영)
    float3 diffuseBRDF = kd * albedo * InvPI;

    // specular BRDF (Cook-Torrance 스페큘러 BRDF)
    float3 specularBRDF = (F * D * G) / max(Epsilon, 4.0 * cosNL * cosNV);
    
    
    // 7. 그림자(Shadow) 계산 : PCF(Percentage-Closer Filtering)
    // - PositionShadow 는 Light View Projection 좌표(NDC) 기반 -> 직접 변환해야 함
    float shadowFactor = 1.0f;
    
    float currentShadowDepth = input.PositionShadow.z / input.PositionShadow.w;
    float2 uv = input.PositionShadow.xy / input.PositionShadow.w;
    
    // NDC -> Texture 좌표계 변환
    uv.y = -uv.y; 
    uv = uv * 0.5f + 0.5f; 
    
    // 그림자 맵 범위 내부인지 체크
    if (uv.x >= 0.0f && uv.x <= 1.0f && uv.y >= 0.0f && uv.y <= 1.0f)
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
                float2 sampleUV = uv + float2(x, y) * texelSize;
                sum += txShadowMap.SampleCmpLevelZero(samShadow, sampleUV, currentShadowDepth - depthBias);
            }
        }
        shadowFactor = sum / 9.0f; // 평균값
    }
    
    // 8. 최종 조명 계산
    float3 radiance = vLightColor.rgb; // 광원 색/강도
    float3 Lo = (diffuseBRDF + specularBRDF) * radiance * cosNL * shadowFactor;
    
    // 발광(emissive) 추가
    float3 emissive = SRGBToLinear(emissiveTex.rgb);
    float3 ambient = 0.03f * albedo; // small ambient term recommended

    float3 colorLinear = ambient + Lo + emissive;
    
    // - colorLinear 를 0~1로 제한 (프레임 버퍼는 0~1범위만 표현 가능)
    // - 계산 다 했으니 다시 감마 보정 : linear -> sRGB 
    float3 finalRgb = LinearToSRGB(saturate(colorLinear));
    
    float4 finalColor = float4(finalRgb, opacityTex.a);
    
    return finalColor; 
}