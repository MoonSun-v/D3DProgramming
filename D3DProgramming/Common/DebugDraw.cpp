#include "DebugDraw.h"

#include <algorithm>

using namespace physx;
using namespace DirectX;

namespace
{
    inline void DrawDashedLine(PrimitiveBatch<VertexPositionColor>* batch, FXMVECTOR start, FXMVECTOR end, FXMVECTOR color)
    {
        XMVECTOR delta = XMVectorSubtract(end, start);
        float length = XMVectorGetX(XMVector3Length(delta));

        // 항상 일정 분할: 16등분
        int segments = 16;
        XMVECTOR step = XMVectorScale(delta, 1.0f / segments);

        for (int i = 0; i < segments; i += 2) // 한 칸 띄워 점선
        {
            XMVECTOR p0 = XMVectorAdd(start, XMVectorScale(step, float(i)));
            XMVECTOR p1 = XMVectorAdd(start, XMVectorScale(step, float(i + 1)));
            batch->DrawLine(VertexPositionColor(p0, color), VertexPositionColor(p1, color));
        }
    }

    inline void XM_CALLCONV DrawCube(PrimitiveBatch<VertexPositionColor>* batch,
        CXMMATRIX matWorld,
        FXMVECTOR color,
        bool dashed = false) // 점선 
    {
        static const XMVECTORF32 s_verts[8] =
        {
            { { { -1.f, -1.f, -1.f, 0.f } } },
            { { {  1.f, -1.f, -1.f, 0.f } } },
            { { {  1.f, -1.f,  1.f, 0.f } } },
            { { { -1.f, -1.f,  1.f, 0.f } } },
            { { { -1.f,  1.f, -1.f, 0.f } } },
            { { {  1.f,  1.f, -1.f, 0.f } } },
            { { {  1.f,  1.f,  1.f, 0.f } } },
            { { { -1.f,  1.f,  1.f, 0.f } } }
        };

        static const WORD s_indices[] =
        {
            0, 1, 1, 2, 2, 3, 3, 0,
            4, 5, 5, 6, 6, 7, 7, 4,
            0, 4, 1, 5, 2, 6, 3, 7
        };

        VertexPositionColor verts[8];
        for (size_t i = 0; i < 8; ++i)
        {
            const XMVECTOR v = XMVector3Transform(s_verts[i], matWorld);
            XMStoreFloat3(&verts[i].position, v);
            XMStoreFloat4(&verts[i].color, color);
        }

        if (dashed)
        {
            for (size_t i = 0; i < std::size(s_indices); i += 2)
            {
                DrawDashedLine(batch, XMLoadFloat3(&verts[s_indices[i]].position),
                    XMLoadFloat3(&verts[s_indices[i + 1]].position),
                    color);
            }
        }
        else
        {
            batch->DrawIndexed(D3D_PRIMITIVE_TOPOLOGY_LINELIST, s_indices, static_cast<UINT>(std::size(s_indices)), verts, 8);
        }
    }
}

void XM_CALLCONV DebugDraw::Draw(PrimitiveBatch<VertexPositionColor>* batch,
    const BoundingSphere& sphere,
    FXMVECTOR color,
    bool dashed)
{
    const XMVECTOR origin = XMLoadFloat3(&sphere.Center);
    const float radius = sphere.Radius;

    const XMVECTOR xaxis = XMVectorScale(g_XMIdentityR0, radius);
    const XMVECTOR yaxis = XMVectorScale(g_XMIdentityR1, radius);
    const XMVECTOR zaxis = XMVectorScale(g_XMIdentityR2, radius);

    //DrawRing(batch, origin, xaxis, zaxis, color);
    //DrawRing(batch, origin, xaxis, yaxis, color);
    //DrawRing(batch, origin, yaxis, zaxis, color);

    constexpr size_t c_ringSegments = 32;
    for (size_t i = 0; i < 3; ++i)
    {
        XMVECTOR major = (i == 0) ? xaxis : (i == 1) ? xaxis : yaxis;
        XMVECTOR minor = (i == 0) ? zaxis : (i == 1) ? yaxis : zaxis;

        if (dashed)
        {
            // 링을 작은 선분으로 나누어 점선
            VertexPositionColor verts[c_ringSegments + 1];
            constexpr float fAngleDelta = XM_2PI / float(c_ringSegments);
            const XMVECTOR cosDelta = XMVectorReplicate(cosf(fAngleDelta));
            const XMVECTOR sinDelta = XMVectorReplicate(sinf(fAngleDelta));
            XMVECTOR incSin = XMVectorZero();
            static const XMVECTORF32 s_initialCos = { { { 1.f,1.f,1.f,1.f } } };
            XMVECTOR incCos = s_initialCos.v;

            for (size_t j = 0; j < c_ringSegments; ++j)
            {
                XMVECTOR pos = XMVectorMultiplyAdd(major, incCos, origin);
                pos = XMVectorMultiplyAdd(minor, incSin, pos);
                verts[j] = VertexPositionColor(pos, color);

                const XMVECTOR newCos = XMVectorSubtract(XMVectorMultiply(incCos, cosDelta), XMVectorMultiply(incSin, sinDelta));
                const XMVECTOR newSin = XMVectorAdd(XMVectorMultiply(incCos, sinDelta), XMVectorMultiply(incSin, cosDelta));
                incCos = newCos; incSin = newSin;
            }
            verts[c_ringSegments] = verts[0];

            // 점선으로 그리기
            for (size_t j = 0; j < c_ringSegments; j += 1)
            {
                DrawDashedLine(batch, XMLoadFloat3(&verts[j].position), XMLoadFloat3(&verts[j + 1].position), color);
            }
        }
        else
        {
            DrawRing(batch, origin, major, minor, color);
        }
    }
}

void XM_CALLCONV DebugDraw::Draw(PrimitiveBatch<VertexPositionColor>* batch,
    const BoundingBox& box,
    FXMVECTOR color)
{
    XMMATRIX matWorld = XMMatrixScaling(box.Extents.x, box.Extents.y, box.Extents.z);
    const XMVECTOR position = XMLoadFloat3(&box.Center);
    matWorld.r[3] = XMVectorSelect(matWorld.r[3], position, g_XMSelect1110);

    DrawCube(batch, matWorld, color);
}

void XM_CALLCONV DebugDraw::Draw(PrimitiveBatch<VertexPositionColor>* batch,
    const BoundingOrientedBox& obb,
    FXMVECTOR color,
    bool dashed)
{
    XMMATRIX matWorld = XMMatrixRotationQuaternion(XMLoadFloat4(&obb.Orientation));
    const XMMATRIX matScale = XMMatrixScaling(obb.Extents.x, obb.Extents.y, obb.Extents.z);
    matWorld = XMMatrixMultiply(matScale, matWorld);
    const XMVECTOR position = XMLoadFloat3(&obb.Center);
    matWorld.r[3] = XMVectorSelect(matWorld.r[3], position, g_XMSelect1110);

    DrawCube(batch, matWorld, color, dashed);
}

void XM_CALLCONV DebugDraw::Draw(PrimitiveBatch<VertexPositionColor>* batch,
    const BoundingFrustum& frustum,
    FXMVECTOR color)
{
    XMFLOAT3 corners[BoundingFrustum::CORNER_COUNT];
    frustum.GetCorners(corners);

    VertexPositionColor verts[24] = {};
    verts[0].position = corners[0];
    verts[1].position = corners[1];
    verts[2].position = corners[1];
    verts[3].position = corners[2];
    verts[4].position = corners[2];
    verts[5].position = corners[3];
    verts[6].position = corners[3];
    verts[7].position = corners[0];

    verts[8].position = corners[0];
    verts[9].position = corners[4];
    verts[10].position = corners[1];
    verts[11].position = corners[5];
    verts[12].position = corners[2];
    verts[13].position = corners[6];
    verts[14].position = corners[3];
    verts[15].position = corners[7];

    verts[16].position = corners[4];
    verts[17].position = corners[5];
    verts[18].position = corners[5];
    verts[19].position = corners[6];
    verts[20].position = corners[6];
    verts[21].position = corners[7];
    verts[22].position = corners[7];
    verts[23].position = corners[4];

    for (size_t j = 0; j < std::size(verts); ++j)
    {
        XMStoreFloat4(&verts[j].color, color);
    }

    batch->Draw(D3D_PRIMITIVE_TOPOLOGY_LINELIST, verts, static_cast<UINT>(std::size(verts)));
}

void XM_CALLCONV DebugDraw::DrawGrid(PrimitiveBatch<VertexPositionColor>* batch,
    FXMVECTOR xAxis,
    FXMVECTOR yAxis,
    FXMVECTOR origin,
    size_t xdivs,
    size_t ydivs,
    GXMVECTOR color)
{
    xdivs = std::max<size_t>(1, xdivs);
    ydivs = std::max<size_t>(1, ydivs);

    for (size_t i = 0; i <= xdivs; ++i)
    {
        float percent = float(i) / float(xdivs);
        percent = (percent * 2.f) - 1.f;
        XMVECTOR scale = XMVectorScale(xAxis, percent);
        scale = XMVectorAdd(scale, origin);

        const VertexPositionColor v1(XMVectorSubtract(scale, yAxis), color);
        const VertexPositionColor v2(XMVectorAdd(scale, yAxis), color);
        batch->DrawLine(v1, v2);
    }

    for (size_t i = 0; i <= ydivs; i++)
    {
        FLOAT percent = float(i) / float(ydivs);
        percent = (percent * 2.f) - 1.f;
        XMVECTOR scale = XMVectorScale(yAxis, percent);
        scale = XMVectorAdd(scale, origin);

        const VertexPositionColor v1(XMVectorSubtract(scale, xAxis), color);
        const VertexPositionColor v2(XMVectorAdd(scale, xAxis), color);
        batch->DrawLine(v1, v2);
    }
}

void XM_CALLCONV DebugDraw::DrawRing(PrimitiveBatch<VertexPositionColor>* batch,
    FXMVECTOR origin,
    FXMVECTOR majorAxis,
    FXMVECTOR minorAxis,
    GXMVECTOR color)
{
    constexpr size_t c_ringSegments = 32;

    VertexPositionColor verts[c_ringSegments + 1];

    constexpr float fAngleDelta = XM_2PI / float(c_ringSegments);
    // Instead of calling cos/sin for each segment we calculate
    // the sign of the angle delta and then incrementally calculate sin
    // and cosine from then on.
    const XMVECTOR cosDelta = XMVectorReplicate(cosf(fAngleDelta));
    const XMVECTOR sinDelta = XMVectorReplicate(sinf(fAngleDelta));
    XMVECTOR incrementalSin = XMVectorZero();
    static const XMVECTORF32 s_initialCos =
    {
        { { 1.f, 1.f, 1.f, 1.f } }
    };
    XMVECTOR incrementalCos = s_initialCos.v;
    for (size_t i = 0; i < c_ringSegments; i++)
    {
        XMVECTOR pos = XMVectorMultiplyAdd(majorAxis, incrementalCos, origin);
        pos = XMVectorMultiplyAdd(minorAxis, incrementalSin, pos);
        XMStoreFloat3(&verts[i].position, pos);
        XMStoreFloat4(&verts[i].color, color);
        // Standard formula to rotate a vector.
        const XMVECTOR newCos = XMVectorSubtract(XMVectorMultiply(incrementalCos, cosDelta), XMVectorMultiply(incrementalSin, sinDelta));
        const XMVECTOR newSin = XMVectorAdd(XMVectorMultiply(incrementalCos, sinDelta), XMVectorMultiply(incrementalSin, cosDelta));
        incrementalCos = newCos;
        incrementalSin = newSin;
    }
    verts[c_ringSegments] = verts[0];

    batch->Draw(D3D_PRIMITIVE_TOPOLOGY_LINESTRIP, verts, c_ringSegments + 1);
}

void XM_CALLCONV DebugDraw::DrawRay(PrimitiveBatch<VertexPositionColor>* batch,
    FXMVECTOR origin,
    FXMVECTOR direction,
    bool normalize,
    FXMVECTOR color)
{
    VertexPositionColor verts[3];
    XMStoreFloat3(&verts[0].position, origin);

    XMVECTOR normDirection = XMVector3Normalize(direction);
    XMVECTOR rayDirection = (normalize) ? normDirection : direction;

    XMVECTOR perpVector = XMVector3Cross(normDirection, g_XMIdentityR1);

    if (XMVector3Equal(XMVector3LengthSq(perpVector), g_XMZero))
    {
        perpVector = XMVector3Cross(normDirection, g_XMIdentityR2);
    }
    perpVector = XMVector3Normalize(perpVector);

    XMStoreFloat3(&verts[1].position, XMVectorAdd(rayDirection, origin));
    perpVector = XMVectorScale(perpVector, 0.0625f);
    normDirection = XMVectorScale(normDirection, -0.25f);
    rayDirection = XMVectorAdd(perpVector, rayDirection);
    rayDirection = XMVectorAdd(normDirection, rayDirection);
    XMStoreFloat3(&verts[2].position, XMVectorAdd(rayDirection, origin));

    XMStoreFloat4(&verts[0].color, color);
    XMStoreFloat4(&verts[1].color, color);
    XMStoreFloat4(&verts[2].color, color);

    batch->Draw(D3D_PRIMITIVE_TOPOLOGY_LINESTRIP, verts, 2);
}

// Raycast 전용
void XM_CALLCONV DebugDraw::DrawRayDebug(PrimitiveBatch<VertexPositionColor>* batch,
    FXMVECTOR origin,
    FXMVECTOR direction,
    FXMVECTOR color,
    bool normalize)
{
    if (!batch) return;
    XMVECTOR rayDir = normalize ? XMVector3Normalize(direction) : direction;
    XMVECTOR end = XMVectorAdd(origin, rayDir);

    VertexPositionColor verts[2];
    XMStoreFloat3(&verts[0].position, origin);
    XMStoreFloat3(&verts[1].position, end);
    XMStoreFloat4(&verts[0].color, color);
    XMStoreFloat4(&verts[1].color, color);

    batch->Draw(D3D_PRIMITIVE_TOPOLOGY_LINELIST, verts, 2);
}


void XM_CALLCONV DebugDraw::DrawTriangle(PrimitiveBatch<VertexPositionColor>* batch,
    FXMVECTOR pointA,
    FXMVECTOR pointB,
    FXMVECTOR pointC,
    GXMVECTOR color)
{
    VertexPositionColor verts[4];
    XMStoreFloat3(&verts[0].position, pointA);
    XMStoreFloat3(&verts[1].position, pointB);
    XMStoreFloat3(&verts[2].position, pointC);
    XMStoreFloat3(&verts[3].position, pointA);

    XMStoreFloat4(&verts[0].color, color);
    XMStoreFloat4(&verts[1].color, color);
    XMStoreFloat4(&verts[2].color, color);
    XMStoreFloat4(&verts[3].color, color);

    batch->Draw(D3D_PRIMITIVE_TOPOLOGY_LINESTRIP, verts, 4);
}

void XM_CALLCONV DebugDraw::DrawQuad(PrimitiveBatch<VertexPositionColor>* batch,
    FXMVECTOR pointA,
    FXMVECTOR pointB,
    FXMVECTOR pointC,
    GXMVECTOR pointD,
    HXMVECTOR color)
{
    VertexPositionColor verts[5];
    XMStoreFloat3(&verts[0].position, pointA);
    XMStoreFloat3(&verts[1].position, pointB);
    XMStoreFloat3(&verts[2].position, pointC);
    XMStoreFloat3(&verts[3].position, pointD);
    XMStoreFloat3(&verts[4].position, pointA);

    XMStoreFloat4(&verts[0].color, color);
    XMStoreFloat4(&verts[1].color, color);
    XMStoreFloat4(&verts[2].color, color);
    XMStoreFloat4(&verts[3].color, color);
    XMStoreFloat4(&verts[4].color, color);

    batch->Draw(D3D_PRIMITIVE_TOPOLOGY_LINESTRIP, verts, 5);
}

void XM_CALLCONV DebugDraw::DrawCapsule(
    PrimitiveBatch<VertexPositionColor>* batch,
    const PxVec3& position,
    float radius,
    float height,
    FXMVECTOR color,
    const PxQuat& rotation,
    bool dashed)
{
    // 캡슐 중심선 Y축 기준
    XMVECTOR pos = XMVectorSet(position.x, position.y, position.z, 1.0f);
    XMMATRIX rotMat = XMMatrixRotationQuaternion(XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&rotation)));

    const int segments = 16;
    float halfHeight = height * 0.5f;

    // 원통 부분
    for (int i = 0; i < segments; ++i)
    {
        float angle0 = (2 * XM_PI / segments) * i;
        float angle1 = (2 * XM_PI / segments) * (i + 1);

        XMVECTOR p0Top = XMVectorSet(radius * cosf(angle0), halfHeight, radius * sinf(angle0), 1.0f);
        XMVECTOR p1Top = XMVectorSet(radius * cosf(angle1), halfHeight, radius * sinf(angle1), 1.0f);

        XMVECTOR p0Bottom = XMVectorSet(radius * cosf(angle0), -halfHeight, radius * sinf(angle0), 1.0f);
        XMVECTOR p1Bottom = XMVectorSet(radius * cosf(angle1), -halfHeight, radius * sinf(angle1), 1.0f);

        // 월드 변환
        p0Top = XMVector3Transform(p0Top, rotMat) + pos;
        p1Top = XMVector3Transform(p1Top, rotMat) + pos;
        p0Bottom = XMVector3Transform(p0Bottom, rotMat) + pos;
        p1Bottom = XMVector3Transform(p1Bottom, rotMat) + pos;
        
        if (dashed)
        {
            // 상단/하단 원: 점선
            DrawDashedLine(batch, p0Top, p1Top, color);
            DrawDashedLine(batch, p0Bottom, p1Bottom, color);

            // 세로 연결선: 점선
            DrawDashedLine(batch, p0Top, p0Bottom, color);
        }
        else
        {
            batch->DrawLine(VertexPositionColor(p0Top, color), VertexPositionColor(p1Top, color)); // 상단 원
            batch->DrawLine(VertexPositionColor(p0Bottom, color), VertexPositionColor(p1Bottom, color)); // 하단 원
            batch->DrawLine(VertexPositionColor(p0Top, color), VertexPositionColor(p0Bottom, color)); // 측면 연결
        }
    }

    // -----------------------------
    // 반구(Sphere) 추가
    // -----------------------------

    // 캡슐의 로컬 Y축 → 월드 방향
    XMVECTOR up = XMVector3TransformNormal(XMVectorSet(0, 1, 0, 0), rotMat);

    // 위 / 아래 반구 중심
    XMVECTOR topCenter = pos + up * halfHeight;
    XMVECTOR bottomCenter = pos - up * halfHeight;

    // BoundingSphere로 그리기
    BoundingSphere topSphere; XMStoreFloat3(&topSphere.Center, topCenter); topSphere.Radius = radius;
    BoundingSphere bottomSphere; XMStoreFloat3(&bottomSphere.Center, bottomCenter); bottomSphere.Radius = radius;

    // DebugDraw::Draw(BoundingSphere) 사용
    Draw(batch, topSphere, color, dashed);
    Draw(batch, bottomSphere, color, dashed);
}