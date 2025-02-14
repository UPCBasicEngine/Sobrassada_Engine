#include "FrustumPlanes.h"

#include "Geometry/AABB.h"
#include "Geometry/OBB.h"
#include "Math/float3.h"
#include "Math/float4x4.h"

FrustumPlanes::FrustumPlanes()
{
    frustumPlanes.assign(TOTAL_PLANES, float4::zero);
}

FrustumPlanes::~FrustumPlanes()
{
    frustumPlanes.clear();
}

void FrustumPlanes::UpdateFrustumPlanes(const float4x4& viewMatrix, const float4x4& projectionMatrix)
{
    float4x4 vpMatrix = projectionMatrix * viewMatrix;

    for (int i = 0; i < TOTAL_PLANES; ++i)
    {

        // Left plane
        if (i == 0)
        {
            frustumPlanes[i].x = vpMatrix[3][0] + vpMatrix[0][0];
            frustumPlanes[i].y = vpMatrix[3][1] + vpMatrix[0][1];
            frustumPlanes[i].z = vpMatrix[3][2] + vpMatrix[0][2];
            frustumPlanes[i].w = vpMatrix[3][3] + vpMatrix[0][3];
        }
        // Right plane
        else if (i == 1)
        {
            frustumPlanes[i].x = vpMatrix[3][0] - vpMatrix[0][0];
            frustumPlanes[i].y = vpMatrix[3][1] - vpMatrix[0][1];
            frustumPlanes[i].z = vpMatrix[3][2] - vpMatrix[0][2];
            frustumPlanes[i].w = vpMatrix[3][3] - vpMatrix[0][3];
        }
        // Bottom plane
        else if (i == 2)
        {
            frustumPlanes[i].x = vpMatrix[3][0] + vpMatrix[1][0];
            frustumPlanes[i].y = vpMatrix[3][1] + vpMatrix[1][1];
            frustumPlanes[i].z = vpMatrix[3][2] + vpMatrix[1][2];
            frustumPlanes[i].w = vpMatrix[3][3] + vpMatrix[1][3];
        }
        // Top plane
        else if (i == 3)
        {
            frustumPlanes[i].x = vpMatrix[3][0] - vpMatrix[1][0];
            frustumPlanes[i].y = vpMatrix[3][1] - vpMatrix[1][1];
            frustumPlanes[i].z = vpMatrix[3][2] - vpMatrix[1][2];
            frustumPlanes[i].w = vpMatrix[3][3] - vpMatrix[1][3];
        }
        // Near plane
        else if (i == 4)
        {
            frustumPlanes[i].x = vpMatrix[3][0] + vpMatrix[2][0];
            frustumPlanes[i].y = vpMatrix[3][1] + vpMatrix[2][1];
            frustumPlanes[i].z = vpMatrix[3][2] + vpMatrix[2][2];
            frustumPlanes[i].w = vpMatrix[3][3] + vpMatrix[2][3];
        }
        // Far plane
        else if (i == 5)
        {
            frustumPlanes[i].x = vpMatrix[3][0] - vpMatrix[2][0];
            frustumPlanes[i].y = vpMatrix[3][1] - vpMatrix[2][1];
            frustumPlanes[i].z = vpMatrix[3][2] - vpMatrix[2][2];
            frustumPlanes[i].w = vpMatrix[3][3] - vpMatrix[2][3];
        }

        frustumPlanes[i].Normalize4();
    }
}

bool FrustumPlanes::Intersects(const AABB& boundingBox) const
{
    float3 corners[8];
    boundingBox.GetCornerPoints(corners);

    return CheckInsideFrustum(corners);
}

bool FrustumPlanes::Intersects(const OBB& boundingBox) const
{
    float3 corners[8];
    boundingBox.GetCornerPoints(corners);

    return CheckInsideFrustum(corners);
}

bool FrustumPlanes::CheckInsideFrustum(const float3 (&corners)[8]) const
{

    for (int plane = 0; plane < 6; ++plane)
    {
        bool allOutside = true;
        for (int corner = 0; corner < 8; ++corner)
        {
            if (PointInPlane(corners[corner], frustumPlanes[plane]))
            {
                allOutside = false;
                break;
            }
        }
        if (allOutside)
        {
            return false;
        }
    }
    return true;
}

bool FrustumPlanes::PointInPlane(const float3& point, const float4& plane) const
{
    return (plane.x * point.x + plane.y * point.y + plane.z * point.z + plane.w) >= 0.0f;
}
