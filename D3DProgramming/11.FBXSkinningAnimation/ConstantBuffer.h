#pragma once
#include "../Common/Helper.h"

__declspec(align(16))
struct ConstantBuffer
{
	Matrix mWorld;			// ���� ��ȯ ��� : 64bytes
	Matrix mView;			// �� ��ȯ ���   : 64bytes
	Matrix mProjection;		// ���� ��ȯ ��� : 64bytes

	Vector4 vLightDir;		// ���� ����
	Vector4 vLightColor;	// ���� ����
	Vector4 vOutputColor;	// ��� ���� 

	Vector4 vEyePos;		// ī�޶� ��ġ

	Vector4 vAmbient;		// ��Ƽ���� Ambient
	Vector4 vDiffuse;		// ��Ƽ���� Diffuse
	Vector4 vSpecular;		// ��Ƽ���� Specular

	float   fShininess = 40.0f;   // ��¦�� ����
	float	gIsRigid = 1;      // 1: Rigid, 0: Skinned
	float	gRefBoneIndex = 0; // �������� �� ���� �� �ε���
	float	pad1[1];			// 16����Ʈ ����
};