#include "14.Shared.hlsli"

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------

// 최종 색 계산 + 그림자 샘플링 

float3 EncodeNormal(float3 N)
{
    return N * 0.5 + 0.5;
}
float3 DecodeNormal(float3 N)
{
    return N * 2 - 1;
}

float4 main(PS_INPUT input) : SV_Target
{
    // 1. 텍스처 샘플링 : C++에서 없으면 기본 텍스쳐 들어가도록 했음 
    float4 texDiffColor = txDiffuse.Sample(samLinear, input.Tex);
    float4 texNormColor = txNormal.Sample(samLinear, input.Tex);
    float4 texSpecColor = txSpecular.Sample(samLinear, input.Tex);
    float4 texEmissive = txEmissive.Sample(samLinear, input.Tex);
    float4 texOpacColor = txOpacity.Sample(samLinear, input.Tex);

    
    // 2. Opacity Clipping (투명도 테스트) -> DepthBuffer, 알파 블렌딩 필요 없음 
    // 투명도 값이 0.5보다 작으면 픽셀을 그리지 않고 버림
    clip(texOpacColor.a - 0.5f); 

    
    // 3. Normal 계산 (TBN 공간에서 월드 공간으로)
    float3 norm = normalize(input.Norm);
    float3 tan = normalize(input.Tangent);
    float3 binorm = normalize(input.Binormal);
    float3x3 tbnMatrix = float3x3(tan, binorm, norm);
    
    // 노멀 맵에서 읽은 색상을 실제 노멀 벡터로 디코딩
    float3 texNorm = DecodeNormal(texNormColor.rgb);
    float3 worldNorm = normalize(mul(texNorm, tbnMatrix));

    
    // 4. Ambient ( diffuse 색상 * 머티리얼 ambient 값 * 라이트 색상 )
    float4 ambient = texDiffColor * vAmbient * vLightColor;

    
    // 5. Diffuse (Lambert)
    float NdotL = max(dot(worldNorm, -vLightDir.xyz), 0.0f);
    float4 diffuse = texDiffColor * vLightColor * NdotL;

    
    // 6. Specular (Blinn-Phong)
    float3 viewDir = normalize(vEyePos.xyz - input.WorldPos);
    float3 halfVector = normalize(-vLightDir.xyz + viewDir);
    float specularFactor = max(dot(worldNorm, halfVector), 0.0f);
    float4 specular = texSpecColor * vSpecular * vLightColor * pow(specularFactor, fShininess);

    
    // 7. Emissive : 자체 발광!
    float4 emissive = texEmissive;


    // [ Shadow 처리 : PCF 사용 ] : 광원NDC 좌표계에서의 좌표는 계산해주지 않으므로 계산한다.
    float currentShadowDepth = input.PositionShadow.z / input.PositionShadow.w;
    float2 uv = input.PositionShadow.xy / input.PositionShadow.w;
    
    // NDC -> Texture 좌표계 변환
    uv.y = -uv.y; // y는 반대 
    uv = uv * 0.5f + 0.5f; // -1 에서 1을 0~1로 변환

    float shadowFactor = 1.0f;
    const float depthBias = 0.005f;
    float texelSize = 1.0 / 8192.0; // ShadowMapSize = 8192.0; // ShadowMap 해상도

    if (uv.x >= 0.0f && uv.x <= 1.0f && uv.y >= 0.0f && uv.y <= 1.0f)
    {
        // 3x3 오프셋
        float2 offsets[9] =
        {
            float2(-1, -1), float2(0, -1), float2(1, -1),
                float2(-1, 0), float2(0, 0), float2(1, 0),
                float2(-1, 1), float2(0, 1), float2(1, 1)
        };
         
        shadowFactor = 0.0f; // 초기화 

        for (int i = 0; i < 9; i++)
        {
            float2 sampleUV = uv + offsets[i] * texelSize;
            shadowFactor += txShadowMap.SampleCmpLevelZero(samShadow, sampleUV, currentShadowDepth - depthBias);
        }
        shadowFactor /= 9.0f; // 평균값
    }

    // 그림자 적용 (diffuse + specular)
    diffuse *= shadowFactor;
    specular *= shadowFactor;
    
    // 8. 최종 색상 합산
    float4 finalColor = saturate(diffuse + ambient + specular + emissive);
    
    // Alpha 채널은 opacity 텍스처 사용
    finalColor.a = texOpacColor.a;

    return finalColor;
}






