#pragma once
#include "../Common/Helper.h"

__declspec(align(16))
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
	float	gIsRigid = 1;         // 1: Rigid, 0: Skinned
	float	pad1[2];			  // 16바이트 정렬
};


struct OutlineBuffer
{
	XMFLOAT4 OutlineColor;
	float OutlineThickness;
	XMFLOAT3 pad; // 16바이트 정렬
};