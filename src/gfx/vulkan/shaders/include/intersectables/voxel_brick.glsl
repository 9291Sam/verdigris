#ifndef SRC_GFX_VULKAN_SHADERS_INCLUDE_INTERSECTABLES_VOXEL_BRICK_GLSL
#define SRC_GFX_VULKAN_SHADERS_INCLUDE_INTERSECTABLES_VOXEL_BRICK_GLSL

#include "voxel.glsl"

const uint VoxelBrick_EdgeLength = 12;
struct VoxelBrick
{
    Voxel[VoxelBrick_EdgeLength][VoxelBrick_EdgeLength]
         [VoxelBrick_EdgeLength] voxels;
};

// IntersectionResult VoxelBrick_tryIntersect(const in Brick self, )

#endif // SRC_GFX_VULKAN_SHADERS_INCLUDE_INTERSECTABLES_VOXEL_BRICK_GLSL