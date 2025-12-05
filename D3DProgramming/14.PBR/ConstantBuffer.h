#pragma once
#include "../Common/Helper.h"

struct ConstantBuffer
{
	Matrix mWorld;			// 월드 변환 행렬 : 64bytes
	Matrix mView;			// 뷰 변환 행렬   : 64bytes
	Matrix mProjection;		// 투영 변환 행렬 : 64bytes

	Vector4 vLightDir;		// 광원 방향
	Vector4 vLightColor;	// 광원 색상

	Vector4 vEyePos;		// 카메라 위치

	Vector4 gMetallicMultiplier;
	Vector4 gRoughnessMultiplier;

	int		gIsRigid = 1;     // 1: Rigid, 0: Skinned
	float	pad[3];			  // 16바이트 정렬
};


// ShadowMap생성 Pass에서 사용ㅎ 
struct ShadowConstantBuffer
{
	Matrix mWorld;          // 모델 -> 월드
	Matrix mLightView;      // 광원 시점 View
	Matrix mLightProjection;// 광원 시점 Projection
};