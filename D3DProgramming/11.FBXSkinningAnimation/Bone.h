#pragma once
#include <string>
#include <DirectXMath.h>
#include "Animation.h"  // BoneAnimation ������

using namespace DirectX;

// [0] pelvis : ParentIndex = -1
// [1] spine : ParentIndex = 0

class Bone
{
public:
    // [ ��ȯ ������ ]
    XMMATRIX m_Local;   // �θ� ���� ��� ��ȯ
    XMMATRIX m_Model;   // �� ���� ���� ���� ��ȯ

    // [ ���̷��� ���� ]
    int m_ParentIndex = -1; // �θ� �� �ε���
    int m_Index = -1;       // �� �ε���
    std::string m_Name;     // �� �̸� (FBX ����)

    // [ �ִϸ��̼� ������ ]
    const class BoneAnimation* m_pBoneAnimation = nullptr; // �� ���� �����Ǵ� �ִϸ��̼� Ʈ��

public:
    Bone() : m_Local(XMMatrixIdentity()), m_Model(XMMatrixIdentity())
    {
    }
};

