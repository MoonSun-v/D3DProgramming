#include "15.Shared.hlsli"

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------

// 인코딩/디코딩 
float3 EncodeNormal(float3 N) { return N * 0.5 + 0.5; }
float3 DecodeNormal(float3 N) { return N * 2.0 - 1.0; }


// 상수 값 
static const float PI = 3.14159265;     // 원주율
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


// Returns number of mipmap levels for specular IBL environment map.
uint querySpecularTextureLevels()
{
    uint width, height, levels;
    txIBL_Specular.GetDimensions(0, width, height, levels);
    return levels;
}


// sRGB -> Linear 
float3 SRGBToLinear(float3 c) { return pow(c, 2.2f); }

// Linear -> sRGB 
float3 LinearToSRGB(float3 c) { return pow(c, 1.0f / 2.2f); }



// --------------------------------------------------------------------------
// PBR + IBL 
// --------------------------------------------------------------------------

float4 main(PS_INPUT input) : SV_Target
{
    // ----------------------------------------------------------------------
    // 1. 텍스처 샘플링 : C++에서 없으면 기본 텍스쳐 들어가도록 했음 
    // ----------------------------------------------------------------------
    float4 baseColorTex = txBaseColor.Sample(samLinear, input.Tex);
    float4 normalTex = txNormal.Sample(samLinear, input.Tex);
    float metalTex = txMetallic.Sample(samLinear, input.Tex).r;
    float roughTex = txRoughness.Sample(samLinear, input.Tex).r;
    float4 emissiveTex = txEmissive.Sample(samLinear, input.Tex);
    float4 opacityTex = txOpacity.Sample(samLinear, input.Tex);

    
    // ----------------------------------------------------------------------
    // 2. Opacity Clipping (투명도 테스트) -> DepthBuffer, 알파 블렌딩 필요 없음 
    // ----------------------------------------------------------------------
    clip(opacityTex.a - 0.5f); 

    
    // ----------------------------------------------------------------------
    // 3. Normal Mapping (TBN Space)
    // ----------------------------------------------------------------------
    float3 N = normalize(input.Norm);
    float3 T = normalize(input.Tangent);
    float3 B = normalize(input.Binormal);

    float3x3 TBN = float3x3(T, B, N);
    float3 normalMap = DecodeNormal(normalTex.rgb);
    
    if (useTexture_Normal == 1)
    {
        N = normalize(mul(normalMap, TBN));
    }
        
    
    // ----------------------------------------------------------------------
    // 4. View / Light / Half 벡터 계산 
    // ----------------------------------------------------------------------
    float3 V = normalize(vEyePos.xyz - input.WorldPos); // 시선 방향
    float3 L = normalize(-vLightDir.xyz);               // 광원 방향
    float3 H = normalize(V + L);                        // Half 벡터

    float cosNL = saturate(dot(N, L));
    float cosNV = saturate(dot(N, V));
    float cosNH = saturate(dot(N, H));
    float cosVH = saturate(dot(V, H));
    
    
    // ----------------------------------------------------------------------
    // 5. Material 파라미터 
    // ----------------------------------------------------------------------
    float3 albedo = useTexture_BaseColor ? SRGBToLinear(baseColorTex.rgb) : SRGBToLinear(manualBaseColor.rgb);
    float metallic = useTexture_Metallic ? saturate(metalTex * gMetallicMultiplier.x) : saturate(gMetallicMultiplier.x); 
    float roughness = useTexture_Roughness ? saturate(roughTex * gRoughnessMultiplier.x) : saturate(gRoughnessMultiplier.x); 
    roughness = max(0.15, roughness); // 거칠기(roughness)는 최소값 유지 (roughness가 0에 가까우면 수학적 특이점 생김)
    
    // F0  (비금속은 0.04, 금속은 Albedo 사용)
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);
    
    
    // ----------------------------------------------------------------------    
    // 6. Direct lighting (Cook-Torrance per-light) 
    // ----------------------------------------------------------------------    
    float3  F = fresnelSchlick(F0, cosVH); // Fresnel (Schlick 근사) 
    float   D = ndfGGX(cosNH, roughness);     // NDF
    float   G = G_Smith(cosNL, cosNV, roughness); // Geometry

    // kd:  금속성, F 항에 따른 확산 반사량 감소 
    // float3 kd = lerp(float3(1, 1, 1) - F, float3(0, 0, 0), metallic);
    float3 kd = (1.0 - F) * (1.0 - metallic);
    
    // DiffuseHDR.dds가 Lambertian irradiance 이미 1/PI 포함이라면 -> PI를 곱하면 diffuse IBL이 한 번 더 세짐
    float3 diffuseBRDF = kd * albedo / PI;

    // specular BRDF (Cook-Torrance 스페큘러 BRDF)
    float3 specularBRDF = (F * D * G) / max(Epsilon, 4.0 * cosNL * cosNV);
    
    
    // ----------------------------------------------------------------------
    // 7. Shadow : PCF
    // ----------------------------------------------------------------------
    // PositionShadow 는 Light View Projection 좌표(NDC) 기반 -> 직접 변환해야 함
    float shadowFactor = 1.0f;
    float currentShadowDepth = input.PositionShadow.z / input.PositionShadow.w;
    
    // NDC -> Texture 좌표계 변환
    float2 uv = input.PositionShadow.xy / input.PositionShadow.w;
    uv.y = -uv.y; 
    uv = uv * 0.5f + 0.5f; 
    
    if (uv.x >= 0.0f && uv.x <= 1.0f && uv.y >= 0.0f && uv.y <= 1.0f) // 그림자 맵 범위 내부인지 체크
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
    
    
    
    // ----------------------------------------------------------------------
    // 8. Direct lighting 출력 조명 계산
    // ----------------------------------------------------------------------
    float3 lightRadiance = vLightColor.rgb; // * 0.6f; // 광원 색/강도 : IBL+PBR 환경에서 직광 1.0은 안쓴다고 함 
    float3 DirectLight = (diffuseBRDF + specularBRDF) * lightRadiance * cosNL * shadowFactor;
    
    
    
    // ----------------------------------------------------------------------
    // 9. IBL : (Diffuse + Specular) split-sum approximation 
    // ----------------------------------------------------------------------
    
    float3 IndirectLight_IBL = float3(0.0f, 0.0f, 0.0f);

    if (useIBL == 1)
    {
        // [ Diffuse IBL ] (Irradiance) -----------------------------------------
        float3 irradiance = txIBL_Diffuse.Sample(samLinearIBL, N).rgb;
    
        float3 F_IBL = fresnelSchlick(F0, cosNV); // IBL용 Fresnel 사용: cosNV를 사용 (view angle)
        // float3 kd_IBL = lerp(1.0 - F_IBL, 0.0, metallic); // 금속이면 diffuse 사라짐
        float3 kd_IBL = (1.0 - F_IBL) * (1.0 - metallic);
    
        float3 diffuseIBL = kd_IBL * albedo * irradiance * 0.2;

    
        // [ Specular IBL ] (Prefiltered env + BRDF LUT) -------------------------
        // float3 R = 2.0 * cosNV * N - V; // 반사벡터 
        float3 R = reflect(-V, N);
    
        // sampleLevel: roughness [0..1] -> LOD [0..mipCount-1]
        uint specularTextureLevels = querySpecularTextureLevels(); //  텍스쳐의 최대 LOD 개수를 구한다.	
        float3 specularIrradiance = txIBL_Specular.SampleLevel(samLinearIBL, R, roughness * specularTextureLevels).rgb;
	    // 시선->노말 반사벡터로 샘플링하여 거칠기로 LOD적용된 specularIrradiance 를 구한다.

        float2 brdf = txIBL_BRDF_LUT.Sample(samClampIBL, float2(cosNV, roughness)).rg;
    
        float3 specularIBL = specularIrradiance * (F0 * brdf.x + brdf.y) * 0.1f; // prefilter * (F0 * A + B)
    
        IndirectLight_IBL = diffuseIBL + specularIBL;
    }
    
    
    // ----------------------------------------------------------------------
    // 10. 최종 조명  : 기존 하드코딩 ambient 제거하고 IBL 사용
    // ----------------------------------------------------------------------
    float3 emissive = SRGBToLinear(emissiveTex.rgb);
    
    float3 colorLinear = DirectLight + IndirectLight_IBL + emissive;
    
    // - colorLinear 를 0~1로 제한 (프레임 버퍼는 0~1범위만 표현 가능)
    // - 계산 다 했으니 다시 감마 보정 : linear -> sRGB 
    float3 finalRgb = LinearToSRGB(saturate(colorLinear));
    float4 finalColor = float4(finalRgb, opacityTex.a);
    
    return finalColor; 
}