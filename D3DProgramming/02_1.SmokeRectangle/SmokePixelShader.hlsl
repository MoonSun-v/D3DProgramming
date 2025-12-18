#include <Shared.fxh>

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------

// [ Hash 함수 : 임의의 난수값 생성 ]
//  0~1 사이의 pseudo-random 값 반환
//  - dot(p, float2(12.9898, 78.233)) : 좌표 p를 고유한 값으로 변환
//  - sin()와 곱셈으로 난수처럼 보이게 만든다
//  - frac() : 소수점 이하만 취함, 0~1 범위로 맞춤
float Hash(float2 p)
{
    return frac(sin(dot(p, float2(12.9898, 78.233))) * 43758.5453);
}

// [ Noise 함수 : 2D Perlin 스타일 노이즈 ]
//  uv 좌표를 넣으면 부드러운 랜덤 값을 반환
float Noise(float2 uv)
{
    float2 i = floor(uv);   // 현재 좌표의 정수 부분 (그리드 위치)
    float2 f = frac(uv);    // 소수 부분 (그리드 내 위치, 0~1)

    // 4개의 꼭지점에서 난수값을 가져옴
    float a = Hash(i); // 왼쪽 아래
    float b = Hash(i + float2(1, 0)); // 오른쪽 아래
    float c = Hash(i + float2(0, 1)); // 왼쪽 위
    float d = Hash(i + float2(1, 1)); // 오른쪽 위

    // 부드러운 보간 계수 생성 (smoothstep)
    float2 u = f * f * (3.0 - 2.0 * f);

    // bilinear interpolation (사각형 내부를 부드럽게 보간)
    float lerp_x0 = lerp(a, b, u.x); // 아래쪽
    float lerp_x1 = lerp(c, d, u.x); // 위쪽
    return lerp(lerp_x0, lerp_x1, u.y); // 위아래 보간 → 최종 값
}

// [ FBM (Fractional Brownian Motion) : 여러 레이어의 Noise 합성 ]
//  입력 좌표 uv에 대해 여러 주파수/진폭의 노이즈를 합쳐 자연스러운 패턴 생성
float FBM(float2 uv)
{
    float value = 0.0; // 최종 fbm 값
    float amplitude = 0.5; // 초기 진폭
    float frequency = 1.0; // 초기 주파수

    // 여러 옥타브(Noise 레이어)를 합성
    for (int i = 0; i < 4; i++)
    {
        value += amplitude * Noise(uv * frequency); // 각 레이어 Noise 합치기
        frequency *= 2.0; // 다음 레이어는 주파수를 2배로 -> 더 세밀한 패턴
        amplitude *= 0.5; // 진폭은 반으로 줄여서 영향력 감소
    }
    return value;
}


float4 main(PS_INPUT input) : SV_TARGET
{
    //=====================================================================
    // [ 단순 FBM 기반 연기 ]
    //=====================================================================
    float2 uv = input.uv; // 정점 셰이더에서 전달된 UV 좌표 (0~1 범위)

    float slowTime = time * 0.2; // 연기 움직임을 느리게 하기 위해 시간 스케일 조절

    // Domain Warping : UV를 FBM으로 살짝 왜곡 (연기가 일렁이는 패턴)
    float2 warp = 0.5 * float2(FBM(uv + slowTime), FBM(uv - slowTime)); // X방향, Y방향
   
    float density = FBM(uv + warp); // 왜곡된 UV에 다시 FBM 적용 -> 연기 밀도 계산
    
    float4 smokeColor = float4(0.8, 0.8, 0.8, density); // 연기 색상 (회색) + 밀도에 따른 투명도

    return smokeColor;
}
