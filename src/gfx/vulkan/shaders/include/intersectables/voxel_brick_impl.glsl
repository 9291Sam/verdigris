#ifndef SRC_GFX_VULKAN_SHADERS_INCLUDE_INTERSECTABLES_VOXEL_BRICK_IMPL_GLSL
#define SRC_GFX_VULKAN_SHADERS_INCLUDE_INTERSECTABLES_VOXEL_BRICK_IMPL_GLSL

#ifndef VOXEL_BRICK_IMPL_ARRAY
#error VOXEL_BRICK_IMPL_ARRAY must be defined
#endif //  VOXEL_BRICK_IMPL_ARRAY

IntersectionResult
VoxelBrick_tryIntersect2(const uint offset, const vec3 cornerPos, const Ray ray)
{
    Cube boundingCube;
    boundingCube.center = cornerPos
                        + vec3(
                              VoxelBrick_EdgeLength / 2,
                              VoxelBrick_EdgeLength / 2,
                              VoxelBrick_EdgeLength / 2);
    // +1 is for the fact that the voxels are centered at 0.5 offsets
    boundingCube.edge_length = VoxelBrick_EdgeLength + 1;

    ivec3 voxelStartIndexChecked;

    if (!Cube_contains(boundingCube, ray.origin))
    {
        IntersectionResult result = Cube_tryIntersect(boundingCube, ray);

        // The ray starts doesn't even hit this brick, it's a miss
        if (!result.intersection_occurred)
        {
            return IntersectionResult_getMiss();
        }
        else // the brick does get hit by the ray
        {
            voxelStartIndexChecked = ivec3(result.maybe_hit_point - cornerPos);
        }
    }
    else
    {
        voxelStartIndexChecked =
            ivec3((ray.origin - cornerPos) / VoxelBrick_EdgeLength);
    }

    // if (Voxel_isVisible(thisVoxel))
    // {
    //     Sphere cube;
    //     cube.center = voxelStartIndexChecked * 1.0 + cornerPos;
    //     cube.radius = 0.5;

    //     return Sphere_tryIntersect(cube, ray);
    // }

    // Assuming the following are defined:
    vec3 rayDir;    // The direction of the ray
    vec3 rayOrigin; // The origin of the ray
    vec3 voxelSize; // The size of the voxel

    vec3  tMax;
    vec3  tDelta;
    ivec3 step;
    ivec3 voxel;

    for (int i = 0; i < 3; ++i)
    {
        // Determine the voxel that the ray starts in
        voxel[i] = int(floor(rayOrigin[i] / voxelSize[i]));

        // Determine the direction of the ray
        if (rayDir[i] < 0)
        {
            step[i]   = -1;
            tMax[i]   = (rayOrigin[i] - voxel[i] * voxelSize[i]) / rayDir[i];
            tDelta[i] = voxelSize[i] / -rayDir[i];
        }
        else
        {
            step[i] = 1;
            tMax[i] =
                ((voxel[i] + 1) * voxelSize[i] - rayOrigin[i]) / rayDir[i];
            tDelta[i] = voxelSize[i] / rayDir[i];
        }
    }

    while (true)
    {
        // Process voxel at voxel.x, voxel.y, voxel.z here
        if (Voxel_isVisible(VOXEL_BRICK_IMPL_ARRAY[offset]
                                .voxels[voxel.x][voxel.y][voxel.z]))
        {
            Sphere cube;
            cube.center = voxel * 1.0 + cornerPos;
            cube.radius = 0.5;

            return Sphere_tryIntersect(cube, ray);
        }

        if (any(lessThan(voxel, ivec3(0))) || any(greaterThan(voxel, ivec3(7))))
        {
            return IntersectionResult_getMiss();
        }

        // Move to next voxel
        if (tMax.x < tMax.y)
        {
            if (tMax.x < tMax.z)
            {
                voxel.x += step.x;
                tMax.x += tDelta.x;
            }
            else
            {
                voxel.z += step.z;
                tMax.z += tDelta.z;
            }
        }
        else
        {
            if (tMax.y < tMax.z)
            {
                voxel.y += step.y;
                tMax.y += tDelta.y;
            }
            else
            {
                voxel.z += step.z;
                tMax.z += tDelta.z;
            }
        }
    }
}

IntersectionResult
VoxelBrick_tryIntersect(const uint offset, const vec3 cornerPos, Ray ray)
{
    Cube brickOuterCube;
    brickOuterCube.center = cornerPos
                          + vec3(
                                VoxelBrick_EdgeLength / 2,
                                VoxelBrick_EdgeLength / 2,
                                VoxelBrick_EdgeLength / 2);

    // +1 is for the fact that the voxels arent perfecetly ccentered
    brickOuterCube.edge_length = VoxelBrick_EdgeLength + 1;

    IntersectionResult result = IntersectionResult_getMiss();

    if (!Cube_tryIntersect(brickOuterCube, ray).intersection_occurred
        && !Cube_contains(brickOuterCube, ray.origin))
    {
        return IntersectionResult_getMiss();
    }

    for (uint i = 0; i < VoxelBrick_EdgeLength; ++i)
    {
        for (uint j = 0; j < VoxelBrick_EdgeLength; ++j)
        {
            for (uint k = 0; k < VoxelBrick_EdgeLength; ++k)
            {
                Voxel thisVoxel =
                    VOXEL_BRICK_IMPL_ARRAY[offset].voxels[i][j][k];

                if (Voxel_isVisible(thisVoxel))
                {
                    Sphere cube;
                    cube.center = vec3(i, j, k) * 1.0 + cornerPos;
                    cube.radius = 0.5;

                    propagateIntersection(
                        result, Sphere_tryIntersect(cube, ray));
                }
            }
        }
    }

    return result;
}

#endif // SRC_GFX_VULKAN_SHADERS_INCLUDE_INTERSECTABLES_VOXEL_BRICK_IMPL_GLSL