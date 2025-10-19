#pragma once
#include "../Common/Helper.h"

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
	float   pad[3];
};
