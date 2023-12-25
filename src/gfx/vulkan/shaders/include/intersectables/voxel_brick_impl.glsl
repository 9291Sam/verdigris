#ifndef SRC_GFX_VULKAN_SHADERS_INCLUDE_INTERSECTABLES_VOXEL_BRICK_IMPL_GLSL
#define SRC_GFX_VULKAN_SHADERS_INCLUDE_INTERSECTABLES_VOXEL_BRICK_IMPL_GLSL

#include <util.glsl>

#ifndef VOXEL_BRICK_IMPL_ARRAY
#error VOXEL_BRICK_IMPL_ARRAY must be defined
#endif //  VOXEL_BRICK_IMPL_ARRAY

IntersectionResult
VoxelBrick_tryIntersect(const uint offset, const vec3 cornerPos, const Ray ray)
{
    Cube boundingCube;
    boundingCube.center      = cornerPos + VoxelBrick_EdgeLength / 2;
    boundingCube.edge_length = VoxelBrick_EdgeLength;

    vec3 rayPos;

    if (Cube_contains(boundingCube, ray.origin))
    {
        rayPos = ray.origin - cornerPos;
    }
    else
    {
        IntersectionResult boundingCubeIntersection =
            Cube_tryIntersect(boundingCube, ray);

        if (boundingCubeIntersection.intersection_occurred)
        {
            rayPos = boundingCubeIntersection.maybe_hit_point - cornerPos;
        }
        else
        {
            return IntersectionResult_getMiss();
        }
    }

    // 45deg on all axies + 1 for extra
    const int MAX_RAY_STEPS = int(VoxelBrick_EdgeLength * 3 + 1);

    vec3 rayDir = ray.direction;

    ivec3 localVoxelPosition = ivec3(floor(rayPos + 0.));

    vec3 deltaDist = abs(vec3(length(rayDir)) / rayDir);

    ivec3 rayStep = ivec3(sign(rayDir));

    vec3 sideDist = (sign(rayDir) * (vec3(localVoxelPosition) - rayPos)
                     + (sign(rayDir) * 0.5) + 0.5)
                  * deltaDist;

    bvec3 mask;

    // TODO: remove excessive iterations
    for (int i = 0; i < MAX_RAY_STEPS; i++)
    {
        // For accruacy reasons we do this in two stages
        // If we've traversed COMPLETELY outside of the cube (outside by more
        // than 2) we know its a miss
        if (any(lessThan(localVoxelPosition, ivec3(-1)))
            || any(
                greaterThan(localVoxelPosition, ivec3(VoxelBrick_EdgeLength))))
        {
            return IntersectionResult_getMiss();
        }

        // Otherwise, we may be on the cusp of the cube and only outside by one,
        // but because of floating point errors we may come back in
        // This is the nasty condition we need.
        if (all(greaterThanEqual(localVoxelPosition, ivec3(0)))
            && all(lessThanEqual(
                localVoxelPosition, ivec3(VoxelBrick_EdgeLength - 1)))
            && Voxel_isVisible(
                VOXEL_BRICK_IMPL_ARRAY[offset]
                    .voxels[localVoxelPosition.x][localVoxelPosition.y]
                           [localVoxelPosition.z]))
        {
            Cube cube;
            cube.center =
                localVoxelPosition * Voxel_Size + cornerPos + Voxel_Size / 2.0;
            cube.edge_length = 1.0;

            IntersectionResult result = Cube_tryIntersect(cube, ray);

            result.maybe_color = Voxel_getLinearColor(
                VOXEL_BRICK_IMPL_ARRAY[offset]
                    .voxels[localVoxelPosition.x][localVoxelPosition.y]
                           [localVoxelPosition.z]);

            if (result.intersection_occurred)
            {
                return result;
            }
            // TODO: what to do when we're inside the cube?
            // else
            // {
            //     return IntersectionResult_getError();
            // }
        }

        // I'm not even going to pretend I know what's going on here.
        // Adapted from https://www.shadertoy.com/view/4dX3zl
        mask = lessThanEqual(sideDist.xyz, min(sideDist.yzx, sideDist.zxy));
        sideDist += vec3(mask) * deltaDist;
        localVoxelPosition += ivec3(vec3(mask)) * rayStep;
    }

    return IntersectionResult_getMiss();
}

IntersectionResult
VoxelBrick_tryIntersectNaieve(const uint offset, const vec3 cornerPos, Ray ray)
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
                    Cube cube;
                    cube.center      = vec3(i, j, k) * 1.0 + cornerPos;
                    cube.edge_length = 1.0;

                    IntersectionResult resu = Cube_tryIntersect(cube, ray);

                    resu.maybe_color = Voxel_getLinearColor(thisVoxel);

                    propagateIntersection(result, resu);
                }
            }
        }
    }

    return result;
}

#endif // SRC_GFX_VULKAN_SHADERS_INCLUDE_INTERSECTABLES_VOXEL_BRICK_IMPL_GLSL