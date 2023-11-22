#ifndef SRX_GFX_VULKAN_SHADERS_INCLUDE_INTERSECTABLE_GLSL
#define SRX_GFX_VULKAN_SHADERS_INCLUDE_INTERSECTABLE_GLSL

struct IntersectionResult
{
    bool  intersection_occured;
    float maybe_distance;
    vec3  maybe_normal;
    vec4  maybe_color;
};

IntersectionResult IntersectionResult_getMiss()
{
    IntersectionResult result;
    result.intersection_occured = false;

    return result;
}

void propagateIntersection(
    inout IntersectionResult bestIntersection, const IntersectionResult result)
{
    if (!bestIntersection.intersection_occured
        || (result.intersection_occured
            && result.maybe_distance < bestIntersection.maybe_distance))
    {
        bestIntersection = result;
    }
}

#endif // SRX_GFX_VULKAN_SHADERS_INCLUDE_INTERSECTABLE_GLSL