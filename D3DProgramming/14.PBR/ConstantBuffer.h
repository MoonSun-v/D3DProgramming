#pragma once
#include "../Common/Helper.h"

struct alignas(16) ConstantBuffer
{
	Matrix mWorld;			// 월드 변환 행렬 : 64bytes
	Matrix mView;			// 뷰 변환 행렬   : 64bytes
	Matrix mProjection;		// 투영 변환 행렬 : 64bytes

	Vector4 vLightDir;		// 광원 방향 : 16
	Vector4 vLightColor;	// 광원 색상 : 16

	Vector4 vEyePos;		// 카메라 위치 : 16

	Vector4 gMetallicMultiplier; // 16
	Vector4 gRoughnessMultiplier;// 16

	int	gIsRigid = 1;     // 1: Rigid, 0: Skinned : 4
	int useTexture_BaseColor;	// 4
	int useTexture_Metallic;	// 4
	int useTexture_Roughness;	// 4

	int useTexture_Normal;		// 4
	float pad0;				    // 4

	XMFLOAT4 manualBaseColor;	// 16

	float manualMetallic;		// 4
	float manualRoughness;		// 4
	float pad1[2];				// 8
};


// ShadowMap생성 Pass에서 사용ㅎ 
struct ShadowConstantBuffer
{
	Matrix mWorld;          // 모델 -> 월드
	Matrix mLightView;      // 광원 시점 View
	Matrix mLightProjection;// 광원 시점 Projection
};