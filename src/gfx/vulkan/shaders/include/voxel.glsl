#extension GL_EXT_shader_explicit_arithmetic_types : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int8 : enable

#ifndef SRC_GFX_VULKAN_SHADERS_INCLUDE_VOXEL_GLSL
#define SRC_GFX_VULKAN_SHADERS_INCLUDE_VOXEL_GLSL

#include "util.glsl"

struct Voxel
{
    /// 0          - Invisible / Invalid
    /// [1, 127]   - Translucent
    /// 128        - Opaque
    /// [129, 255] - Emissive
    uint8_t alpha_or_emissive;

    uint8_t srgb_r;
    uint8_t srgb_g;
    uint8_t srgb_b;

    /// 0 - nothing special
    /// 1 - anisotropic?
    /// [2, 255] UB
    uint8_t special;
    uint8_t specular;
    uint8_t roughness;
    uint8_t metallic;
};

#define Voxel_AlphaEmissiveState uint
#define Voxel_Size               1.0

bool                     Voxel_isVisible(const in Voxel);
vec4                     Voxel_getLinearColor(const in Voxel);
Voxel_AlphaEmissiveState Voxel_getAlphaEmissiveState(const in Voxel);

const Voxel_AlphaEmissiveState Voxel_AlphaEmissiveState_Transluscent = 0;
const Voxel_AlphaEmissiveState Voxel_AlphaEmissiveState_Opaque       = 1;
const Voxel_AlphaEmissiveState Voxel_AlphaEmissiveState_Emissive     = 2;

bool Voxel_isVisible(const in Voxel self)
{
    if (self.alpha_or_emissive == 0)
    {
        return false;
    }
    else
    {
        return true;
    }
}

Voxel_AlphaEmissiveState Voxel_getAlphaEmissiveState(const in Voxel self)
{
    if (self.alpha_or_emissive == 128)
    {
        return Voxel_AlphaEmissiveState_Opaque;
    }
    else if (self.alpha_or_emissive < 128)
    {
        return Voxel_AlphaEmissiveState_Transluscent;
    }
    else
    {
        return Voxel_AlphaEmissiveState_Emissive;
    }
}

vec4 Voxel_getLinearColor(const in Voxel self)
{
    return convertSRGBToLinear(
        vec4(
            self.srgb_r,
            self.srgb_g,
            self.srgb_b,
            float(self.alpha_or_emissive) * 2)
        / 255.0);
}

#endif // SRC_GFX_VULKAN_SHADERS_INCLUDE_VOXEL_GLSL