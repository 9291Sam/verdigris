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
            voxelStartIndexChecked = ivec3(result.maybe_hit_point - ray.origin);
        }
    }

    const Voxel thisVoxel =
        VOXEL_BRICK_IMPL_ARRAY[offset]
            .voxels[voxelStartIndexChecked.x][voxelStartIndexChecked.y]
                   [voxelStartIndexChecked.z];

    if (Voxel_isVisible(thisVoxel))
    {
        Sphere cube;
        cube.center = voxelStartIndexChecked * 1.0 + cornerPos;
        cube.radius = 0.5;

        return Sphere_tryIntersect(cube, ray);
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