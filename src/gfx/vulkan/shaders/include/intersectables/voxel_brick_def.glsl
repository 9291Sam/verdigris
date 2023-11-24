#ifndef SRC_GFX_VULKAN_SHADERS_INCLUDE_INTERSECTABLES_VOXEL_BRICK_DEF_GLSL
#define SRC_GFX_VULKAN_SHADERS_INCLUDE_INTERSECTABLES_VOXEL_BRICK_DEF_GLSL

#include <voxel.glsl>

const uint VoxelBrick_EdgeLength = 8;
struct VoxelBrick
{
    Voxel[VoxelBrick_EdgeLength][VoxelBrick_EdgeLength]
         [VoxelBrick_EdgeLength] voxels;
};

#endif // SRC_GFX_VULKAN_SHADERS_INCLUDE_INTERSECTABLES_VOXEL_BRICK_DEF_GLSL