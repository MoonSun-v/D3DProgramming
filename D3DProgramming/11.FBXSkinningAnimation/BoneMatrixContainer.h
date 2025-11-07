#pragma once
#include "../Common/Helper.h"

// vector 사용안하고, 배열 사용.
struct BoneMatrixContainer
{
    static constexpr int MaxBones = 128;    // GPU 상수 버퍼 최대 크기
    Matrix m_Model[128];                    // 각 본의 최종 행렬

    void Clear()
    {
        for (int i = 0; i < MaxBones; ++i)
            m_Model[i] = Matrix::Identity;
    }

    void SetBoneCount(int count)
    {
        if (count > MaxBones) count = MaxBones;
        for (int i = count; i < MaxBones; ++i)
            m_Model[i] = Matrix::Identity;
    }

    void SetMatrix(int index, const Matrix& mat)
    {
        if (index >= 0 && index < MaxBones)
            m_Model[index] = mat;
    }

    const Matrix& GetMatrix(int index) const { return m_Model[index]; }
};
