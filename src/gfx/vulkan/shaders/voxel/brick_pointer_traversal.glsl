#ifndef SRC_GFX_VULKAN_SHADERS_VOXEL_BRICK_POINTER_TRAVERSAL_GLSL
#define SRC_GFX_VULKAN_SHADERS_VOXEL_BRICK_POINTER_TRAVERSAL_GLSL

#ifndef BRICK_POINTER_IMPL_ARRAY
#error BRICK_POINTER_IMPL_ARRAY must be defined
#endif // BRICK_POINTER_IMPL_ARRAY

#ifndef BRICK_POINTER_IMPL_DIMENSION
#error BRICK_POINTER_IMPL_DIMENSION must be defined
#endif // BRICK_POINTER_IMPL_DIMENSION

IntersectionResult traverseBrickPointer(const vec3 cornerPos, const Ray ray)
{
    // Dividing the ray by the the edge length of the brick, traversing, and
    // then multiplying back is actually really useful

    const uint BrickPointerVolumeWidthVoxels =
        BRICK_POINTER_IMPL_DIMENSION * VoxelBrick_EdgeLength;

    Cube boundingCube;
    boundingCube.center =
        cornerPos + (BrickPointerVolumeWidthVoxels * Voxel_Size) / 2.0;
    boundingCube.edge_length = VoxelBrick_EdgeLength;

    vec3 rayPos;

    if (Cube_contains(boundingCube, ray.origin))
    {
        // TODO: where to put the / 8?
        // rayPos = ray.origin - cornerPos;
    }
    else
    {
        IntersectionResult boundingCubeIntersection =
            Cube_tryIntersect(boundingCube, ray);

        if (boundingCubeIntersection.intersection_occurred)
        {
            // TODO: where to put the / 8?
            // rayPos = boundingCubeIntersection.maybe_hit_point - cornerPos;
        }
        else
        {
            return IntersectionResult_getMiss();
        }
    }

    // 45deg on all axies + 1 for extra
    const int MAX_RAY_STEPS = int(BRICK_POINTER_IMPL_DIMENSION * 3 + 1);

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
            // TODO: MaybeVoxelIndex_isIndex
            // && Voxel_isVisible(
            //     VOXEL_BRICK_IMPL_ARRAY[offset]
            //         .voxels[localVoxelPosition.x][localVoxelPosition.y]
            //                [localVoxelPosition.z]))
        {
            Cube cube;
            cube.center =
                // TODO: where to put the / 8
                // localVoxelPosition * Voxel_Size + cornerPos + Voxel_Size
                // / 2.0;
                cube.edge_length = 1.0; // TODO: larger

            // IntersectionResult result = Cube_tryIntersect(cube, ray); //
            // TODO: * 8

            result.maybe_color = Voxel_getLinearColor(
                VOXEL_BRICK_IMPL_ARRAY[offset]
                    .voxels[localVoxelPosition.x][localVoxelPosition.y]
                           [localVoxelPosition.z]);

            if (result.intersection_occurred)
            {
                return result;
            }
            // TODO: what to do when we're inside the cube?
            else
            {
                return IntersectionResult_getError();
            }
        }

        // I'm not even going to pretend I know what's going on here.
        // Adapted from https://www.shadertoy.com/view/4dX3zl
        mask = lessThanEqual(sideDist.xyz, min(sideDist.yzx, sideDist.zxy));
        sideDist += vec3(mask) * deltaDist;
        localVoxelPosition += ivec3(vec3(mask)) * rayStep;
    }

    return IntersectionResult_getMiss();
}

#endif // SRC_GFX_VULKAN_SHADERS_VOXEL_BRICK_POINTER_TRAVERSAL_GLSL