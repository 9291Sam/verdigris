#extension GL_EXT_shader_explicit_arithmetic_types : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int8 : enable

#include "util.glsl"

struct Voxel
{
    uint8_t srgb_r;
    uint8_t srgb_g;
    uint8_t srgb_b;

    /// [0, 127]   - Transluscent
    /// 128        - Opaque
    /// [129, 255] - Emissive
    uint8_t alpha_or_emissive;

    // TODO: there's too much information here
    uint8_t specular;
    uint8_t roughness;
    uint8_t metallic;

    /// 0 - nothing special
    /// 1 - anisotropic
    /// [2, 255] UB
    uint8_t special;
};

#define Voxel_AlphaEmissiveState uint

Voxel_AlphaEmissiveState Voxel_getAlphaEmissiveState(in Voxel);
vec4 Voxel_getColor(in Voxel);

const Voxel_AlphaEmissiveState Voxel_AlphaEmissiveState_Transluscent = 0;
const Voxel_AlphaEmissiveState Voxel_AlphaEmissiveState_Opaque = 1;
const Voxel_AlphaEmissiveState Voxel_AlphaEmissiveState_Emissive = 2;

Voxel_AlphaEmissiveState Voxel_getAlphaEmissiveState(in Voxel self)
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

vec4 Voxel_getColor(in Voxel self)
{
    return convertSRGBToLinear(vec4(
        self.srgb_r,
        self.srgb_g,
        self.srgb_b, 
        float(self.alpha_or_emissive) / 128.0
    ));
}
