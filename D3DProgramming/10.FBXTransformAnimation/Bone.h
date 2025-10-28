#pragma once
#include <string>
#include <DirectXMath.h>
#include "Animation.h"  // BoneAnimation 참조용

using namespace DirectX;

// [0] pelvis : ParentIndex = -1
// [1] spine : ParentIndex = 0

class Bone
{
public:
    // [ 변환 데이터 ]
    XMMATRIX m_Local;   // 부모 기준 상대 변환
    XMMATRIX m_Model;   // 모델 공간 기준 누적 변환

    // [ 스켈레톤 구조 ]
    int m_ParentIndex = -1; // 부모 본 인덱스
    int m_Index = -1;       // 본 인덱스
    std::string m_Name;     // 본 이름 (FBX 노드명)

    // [ 애니메이션 데이터 ]
    const class BoneAnimation* m_pBoneAnimation = nullptr; // 이 본에 대응되는 애니메이션 트랙

public:
    Bone() : m_Local(XMMatrixIdentity()), m_Model(XMMatrixIdentity())
    {
    }
};

