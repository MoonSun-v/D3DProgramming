#include "Animation.h"
#include <algorithm>

// �־��� �ð�(time)�� �ش��ϴ� ���� ��ȯ ��� ���
Matrix BoneAnimation::Evaluate(float time) const
{
    // �ִϸ��̼� Ű�� ��������� ���� ��� ��ȯ (��, ��ȯ ����)
    if (AnimationKeys.empty())
        return Matrix::Identity;

    // �ִϸ��̼� Ű�� �ϳ����� ��� : �״�� ��ȯ
    if (AnimationKeys.size() == 1)
    {
        const AnimationKey& k = AnimationKeys[0];
        return Matrix::CreateScale(k.Scaling)
            * Matrix::CreateFromQuaternion(k.Rotation)
            * Matrix::CreateTranslation(k.Position);
    }

    // 1. ���� �ð�(time)�� �ش��ϴ� Ű ���� ã��
    size_t i = 0;
    while (i + 1 < AnimationKeys.size() && time >= AnimationKeys[i + 1].Time) ++i;
    // time�� ������ Ű���� ū ��� ������ �� Ű�� ���
    if (i + 1 >= AnimationKeys.size()) i = AnimationKeys.size() - 2;

    // ���� ������ �� Ű (���� �����Ӱ� ���� ������)
    const AnimationKey& k0 = AnimationKeys[i];
    const AnimationKey& k1 = AnimationKeys[i + 1];

    // 2. �� Ű ���̿��� ���� ����(t) ���
    float span = k1.Time - k0.Time; // �� Ű ���� �ð� ��
    float t = (span > 0.0f) ? ((time - k0.Time) / span) : 0.0f; // ���� ����
    if (t < 0.0f) t = 0.0f; // 0~1 ������ ����
    if (t > 1.0f) t = 1.0f;

    // 3. ��ġ / ������ / ȸ�� ����
    Vector3 pos = Vector3::Lerp(k0.Position, k1.Position, t);       // �������� 
    Vector3 scl = Vector3::Lerp(k0.Scaling, k1.Scaling, t);         // �������� 
    Quaternion rot = Quaternion::Slerp(k0.Rotation, k1.Rotation, t); // ȸ���� ���鼱������(Slerp)
    rot.Normalize();

    // 4. ���� ��ȯ ��� (S * R * T)
    return Matrix::CreateScale(scl)
        * Matrix::CreateFromQuaternion(rot)
        * Matrix::CreateTranslation(pos);
}

// Evaluate (�����ε� ����) : ��� ��� ���� ��� ��ȯ
void BoneAnimation::Evaluate(float time, Vector3& outPos, Quaternion& outRot, Vector3& outScale) const
{
    // Ű�� ���� ��� �⺻�� ��ȯ
    if (AnimationKeys.empty())
    {
        outPos = Vector3::Zero;
        outRot = Quaternion::Identity;
        outScale = Vector3::One;
        return;
    }

    // Ű�� �ϳ����� ��� �ش� Ű �� ��ȯ
    if (AnimationKeys.size() == 1)
    {
        const AnimationKey& k = AnimationKeys[0];
        outPos = k.Position;
        outRot = k.Rotation;
        outScale = k.Scaling;
        return;
    }

    // 1. ���� �ð��� �´� Ű ���� ã��
    size_t i = 0;
    while (i + 1 < AnimationKeys.size() && time >= AnimationKeys[i + 1].Time) ++i;
    if (i + 1 >= AnimationKeys.size()) i = AnimationKeys.size() - 2;

    const AnimationKey& k0 = AnimationKeys[i];
    const AnimationKey& k1 = AnimationKeys[i + 1];

    // 2. ���� ���� ���
    float span = k1.Time - k0.Time;
    float t = (span > 0.0f) ? ((time - k0.Time) / span) : 0.0f;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    // 3. ���� ����
    outPos = Vector3::Lerp(k0.Position, k1.Position, t);
    outScale = Vector3::Lerp(k0.Scaling, k1.Scaling, t);
    outRot = Quaternion::Slerp(k0.Rotation, k1.Rotation, t);
    outRot.Normalize();
}

// Ư�� ��(bone)�� �־��� �ð�(time)�� ���� ��ȯ ��� ��ȯ
Matrix Animation::GetBoneTransform(int boneIndex, float time) const
{
    // ��ȿ���� ���� �� �ε����� ��� ���� ��� ��ȯ
    if (boneIndex < 0 || boneIndex >= (int)BoneAnimations.size())
        return Matrix::Identity;

    // �ش� �� �ִϸ��̼�
    return BoneAnimations[boneIndex].Evaluate(time);
}