#extension GL_EXT_shader_explicit_arithmetic_types : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int8 : enable

struct Voxel
{
    uint8_t srgb_r;
    uint8_t srgb_g;
    uint8_t srgb_b;

    uint8_t alpha_or_emissive;

    // TODO: there's too much information here
    uint8_t specular;
    uint8_t roughness;
    uint8_t metallic;

    /// 0 - nothing special
    /// 1 - anisotropic?
    /// [2, 255] UB
    uint8_t special;
};
