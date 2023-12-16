#ifndef SRX_GFX_VULKAN_SHADERS_INCLUDE_INTERSECTABLE_GLSL
#define SRX_GFX_VULKAN_SHADERS_INCLUDE_INTERSECTABLE_GLSL

struct IntersectionResult
{
    bool  intersection_occurred;
    float maybe_distance;
    vec3  maybe_hit_point;
    vec3  maybe_normal;
    vec4  maybe_color;
};

IntersectionResult IntersectionResult_getMiss()
{
    IntersectionResult result;
    result.intersection_occurred = false;

    return result;
}

IntersectionResult IntersectionResult_getError()
{
    IntersectionResult result;

    result.intersection_occurred = true;
    result.maybe_distance        = -1.0;
    result.maybe_hit_point       = vec3(0.0);
    result.maybe_normal          = vec3(0.0);
    result.maybe_color           = vec4(1.0, 0.0, 1.0, 1.0);

    return result;
}

void propagateIntersection(
    inout IntersectionResult bestIntersection, const IntersectionResult result)
{
    if (!bestIntersection.intersection_occurred
        || (result.intersection_occurred
            && result.maybe_distance < bestIntersection.maybe_distance))
    {
        bestIntersection = result;
    }
}

#endif // SRX_GFX_VULKAN_SHADERS_INCLUDE_INTERSECTABLE_GLSL