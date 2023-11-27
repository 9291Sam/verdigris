#ifndef SRC_GFX_VULKAN_SHADERS_INCLUDE_INTERSECTABLES_SPHERE_GLSL
#define SRC_GFX_VULKAN_SHADERS_INCLUDE_INTERSECTABLES_SPHERE_GLSL

#include "intersectable.glsl"

struct Sphere
{
    vec3  center;
    float radius;
};

// Stolen straight from https://gamedev.stackexchange.com/a/96487
// It's just so much better than I could write
IntersectionResult Sphere_tryIntersect(const Sphere self, const Ray ray)
{
    const vec3 m = ray.origin - self.center;

    const float b = dot(m, ray.direction);
    const float c = dot(m, m) - self.radius * self.radius;

    // Exit if r’s origin outside s (c > 0) and r pointing away from s (b > 0)
    if (c > 0.0 && b > 0.0)
    {
        return IntersectionResult_getMiss();
    }
    const float discr = b * b - c;

    // A negative discriminant corresponds to ray missing sphere
    if (discr < 0.0)
    {
        return IntersectionResult_getMiss();
    }

    // Ray now found to intersect sphere, compute smallest t value of
    // intersection
    const float t = -b - sqrt(discr);

    // If t is negative, ray started inside sphere so exit
    if (t < 0.0f)
    {
        return IntersectionResult_getMiss();
    }
    const vec3 hitPoint = Ray_at(ray, t);

    IntersectionResult result;
    result.intersection_occurred = true;
    result.maybe_distance        = length(ray.origin - hitPoint);
    result.maybe_hit_point       = hitPoint;
    result.maybe_normal          = normalize(hitPoint - self.center);
    result.maybe_color           = vec4(1.0, 1.0, 1.0, 1.0);

    return result;
}

#endif // SRC_GFX_VULKAN_SHADERS_INCLUDE_INTERSECTABLES_SPHERE_GLSL