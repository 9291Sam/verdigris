#ifndef SRX_GFX_VULKAN_SHADERS_INCLUDE_INTERSECTABLE_GLSL
#define SRX_GFX_VULKAN_SHADERS_INCLUDE_INTERSECTABLE_GLSL

struct IntersectionResult
{
    bool intersection_occured;
    float maybe_distance;
    vec3 maybe_normal;
    vec4 maybe_color;
};

IntersectionResult IntersectionResult_getMiss()
{
    IntersectionResult result;
    result.intersection_occured = false;

    return result;
}

#endif // SRX_GFX_VULKAN_SHADERS_INCLUDE_INTERSECTABLE_GLSL