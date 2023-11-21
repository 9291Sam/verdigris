#ifndef SRC_GFX_VULKAN_SHADERS_INCLUDE_INTERSECTABLES_VOXEL_BRICK_GLSL
#define SRC_GFX_VULKAN_SHADERS_INCLUDE_INTERSECTABLES_VOXEL_BRICK_GLSL

#include "voxel.glsl"

struct VoxelBrick
{
    Voxel[8][8][8] voxels;
}

IntersectionResult VoxelBrick_tryIntersect(in const Brick self,  )

#endif // SRC_GFX_VULKAN_SHADERS_INCLUDE_INTERSECTABLES_VOXEL_BRICK_GLSL