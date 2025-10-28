#pragma once
#include "../Common/Helper.h"

// vector �����ϰ�, �迭 ���.
struct BoneMatrixContainer
{
    static constexpr int MaxBones = 128;  // GPU ��� ���� �ִ� ũ��
    Matrix m_Model[MaxBones];             // �� ���� ���� ���

    void Clear()
    {
        for (int i = 0; i < MaxBones; ++i)
            m_Model[i] = Matrix::Identity;
    }

    void SetBoneCount(int count)
    {
        // count�� MaxBones���� ũ�� �����ϰ� �߶�
        int limit = count;
        if (limit > MaxBones)  limit = MaxBones;

        for (int i = 0; i < limit; ++i)
            m_Model[i] = Matrix::Identity;
    }

    void SetMatrix(int index, const Matrix& mat)
    {
        if (index >= 0 && index < MaxBones)
            m_Model[index] = mat;
    }
};
