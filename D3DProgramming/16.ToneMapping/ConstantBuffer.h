#pragma once
#include "../Common/Helper.h"

// 1: Rigid, 0: Skinned
struct /*alignas(16)*/ ConstantBuffer
{
	Matrix mWorld;			// 월드 변환 행렬 : 64
	Matrix mView;			// 뷰 변환 행렬   : 64
	Matrix mProjection;		// 투영 변환 행렬 : 64

	Vector4 vLightDir;		// 광원 방향 : 16
	Vector4 vLightColor;	// 광원 색상 : 16
	Vector4 vEyePos;		// 카메라 위치 : 16

	Vector4 gMetallicMultiplier; // 16
	Vector4 gRoughnessMultiplier;// 16
	Vector4 manualBaseColor;	 // 16

	float gIsRigid;
	float useTexture_BaseColor;
	float useTexture_Metallic;
	float useTexture_Roughness;

	float useTexture_Normal;
	float useIBL;
	float pad0[2];

	// float pad1[12];   // 12 * 4 = 48 bytes => total 368 bytes  
};


// ShadowMap Pass에서 사용
struct ShadowConstantBuffer
{
	Matrix mWorld;          // 모델 -> 월드
	Matrix mLightView;      // 광원 시점 View
	Matrix mLightProjection;// 광원 시점 Projection
};

// ToneMapping Pass에서 사용
struct ToneMapConstantBuffer
{
	float Exposure;
	float MaxHDRNits;
	float Padding[2];
};