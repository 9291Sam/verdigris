#extension GL_EXT_shader_explicit_arithmetic_types : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int8 : enable

#ifndef SRC_GFX_VULKAN_SHADERS_VOXEL_BRICK_POINTER_GLSL
#define SRC_GFX_VULKAN_SHADERS_VOXEL_BRICK_POINTER_GLSL

struct BrickPointer
{
    uint8_t  niche;
    uint8_t  _padding0;
    uint8_t  _padding1;
    uint8_t  _padding2;
    uint32_t index;
};

bool BrickPointer_isValid(const BrickPointer self)
{
    return self.niche == 0;
}

#endif // SRC_GFX_VULKAN_SHADERS_VOXEL_BRICK_POINTER_GLSL