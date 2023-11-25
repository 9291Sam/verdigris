#ifndef SRC_GFX_VULKAN_SHADERS_INCLUDE_INTERSECTABLES_SPHERE_GLSL
#define SRC_GFX_VULKAN_SHADERS_INCLUDE_INTERSECTABLES_SPHERE_GLSL

#include "intersectable.glsl"

struct Sphere
{
    vec3  center;
    float radius;
};

IntersectionResult Sphere_tryIntersect(const Sphere self, in Ray ray)
{
    // FIXME: whats wrong with this. it works, but why is this inverted and we
    // have a max...
    ray.direction = -ray.direction;

    vec3  oc = ray.origin - self.center;
    float a  = dot(ray.direction, ray.direction);
    float b  = 2.0 * dot(oc, ray.direction);
    float c  = dot(oc, oc) - self.radius * self.radius;

    // Calculate the discriminant to determine if there's an intersection
    float discriminant = b * b - 4.0 * a * c;

    if (discriminant > 0.0)
    {
        // Ray intersects the sphere
        // Calculate the value of t for the two possible intersection points
        float t1 = (-b - sqrt(discriminant)) / (2.0 * a);
        float t2 = (-b + sqrt(discriminant)) / (2.0 * a);

        // Choose the smaller positive t value
        float t = max(t1, t2); // TODO: wait, what??, why is this the max???

        // Calculate the intersection point
        vec3 intersectionPoint = Ray_at(ray, t);

        if (dot(intersectionPoint - ray.origin, ray.direction) > 0.0)
        {
            return IntersectionResult_getMiss();
        }

        // Calculate and return the color based on the intersection point
        IntersectionResult result;
        result.intersection_occurred = true;
        result.maybe_distance        = length(ray.origin - intersectionPoint);
        result.maybe_normal = normalize(intersectionPoint - self.center);
        result.maybe_color  = vec4(1.0, 1.0, 1.0, 1.0);

        return result;
    }
    else
    {
        return IntersectionResult_getMiss();
    }
}

#endif // SRC_GFX_VULKAN_SHADERS_INCLUDE_INTERSECTABLES_SPHERE_GLSL