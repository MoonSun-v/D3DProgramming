#pragma once
#include "../Common/Helper.h"

struct ConstantBuffer
{
	Matrix mWorld;			// 월드 변환 행렬 : 64bytes
	Matrix mView;			// 뷰 변환 행렬   : 64bytes
	Matrix mProjection;		// 투영 변환 행렬 : 64bytes

	Vector4 vLightDir;		// 광원 방향
	Vector4 vLightColor;	// 광원 색상
	Vector4 vOutputColor;	// 출력 색상 

	Vector4 vEyePos;		// 카메라 위치

	Vector4 vAmbient;		// 머티리얼 Ambient
	Vector4 vDiffuse;		// 머티리얼 Diffuse
	Vector4 vSpecular;		// 머티리얼 Specular
	float   fShininess = 40.0f;   // 반짝임 정도
	float   pad[3];
};
