#ifndef SRC_GFX_VULKAN_SHADERS_INCLUDE_INTERSECTABLES_VOXEL_BRICK_IMPL_GLSL
#define SRC_GFX_VULKAN_SHADERS_INCLUDE_INTERSECTABLES_VOXEL_BRICK_IMPL_GLSL

#ifndef VOXEL_BRICK_IMPL_ARRAY
#error VOXEL_BRICK_IMPL_ARRAY must be defined
#endif //  VOXEL_BRICK_IMPL_ARRAY

IntersectionResult VoxelBrick_tryIntersect(uint offset, const Ray ray)
{
    Cube brickOuterCube;
    brickOuterCube.center = vec3(
        VoxelBrick_EdgeLength / 2,
        VoxelBrick_EdgeLength / 2,
        VoxelBrick_EdgeLength / 2);

    brickOuterCube.edge_length = VoxelBrick_EdgeLength + 1;

    IntersectionResult result = IntersectionResult_getMiss();

    if (!Cube_tryIntersect(brickOuterCube, ray).intersection_occurred)
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
                    cube.center      = vec3(i, j, k);
                    cube.edge_length = 1.0;

                    propagateIntersection(result, Cube_tryIntersect(cube, ray));
                }
            }
        }
    }

    return result;
}

#endif // SRC_GFX_VULKAN_SHADERS_INCLUDE_INTERSECTABLES_VOXEL_BRICK_IMPL_GLSL