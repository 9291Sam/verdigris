#ifndef SRC_GFX_VULKAN_SHADERS_INCLUDE_INTERSECTABLES_SPHERE_GLSL
#define SRC_GFX_VULKAN_SHADERS_INCLUDE_INTERSECTABLES_SPHERE_GLSL

#include "intersectable.glsl"

struct Sphere
{
    vec3  center;
    float radius;
};

// IntersectionResult Sphere_tryIntersect(in Sphere self, in Ray ray)
// {
//     // write this function
//     // the miss color can be gotten with the function
//     IntersectionResult_getMiss
// reference
// struct IntersectionResult
// {
//     bool intersection_occured;
//     float maybe_distance;
//     vec3 maybe_normal;
//     vec4 maybe_color;
// };
// // }
// IntersectionResult Sphere_tryIntersect(in Sphere self, in Ray ray)
// {
//     // Compute the vector from the ray's origin to the sphere's center
//     vec3 oc = self.center - ray.origin;

//     // Compute coefficients for the quadratic equation
//     float a = dot(ray.direction, ray.direction);
//     float b = 2.0 * dot(oc, ray.direction);
//     float c = dot(oc, oc) - (self.radius * self.radius);

//     // Compute the discriminant
//     float discriminant = b * b - 4.0 * a * c;

//     // Check if there is a real intersection
//     if (discriminant > 0.0)
//     {
//         // Compute the two possible solutions for t
//         float t1 = (-b - sqrt(discriminant)) / (2.0 * a);
//         float t2 = (-b + sqrt(discriminant)) / (2.0 * a);

//         // Use the smaller positive solution
//         float t = min(t1, t2);

//         vec3 intersection_point = ray.origin + t * ray.direction;

//         // Compute the intersection point and normal
//         IntersectionResult result;
//         result.intersection_occured = true;
//         result.maybe_distance       = length(intersection_point -
//         ray.origin); result.maybe_normal = normalize(intersection_point -
//         self.center); result.maybe_color  = vec4(1.0, 1.0, 1.0, 1.0);

//         return result;
//     }

//     // No real intersection
//     return IntersectionResult_getMiss();
// }
IntersectionResult Sphere_tryIntersect(in Sphere self, in Ray ray)
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
        result.intersection_occured = true;
        result.maybe_distance       = length(ray.origin - intersectionPoint);
        result.maybe_normal = normalize(intersectionPoint - self.center);

        return result;
    }
    else
    {
        return IntersectionResult_getMiss();
    }
}

#endif // SRC_GFX_VULKAN_SHADERS_INCLUDE_INTERSECTABLES_SPHERE_GLSL

// vec3  oc = self.center - ray.origin;
// float a  = dot(ray.direction, ray.direction);
// float b  = 2.0 * dot(oc, ray.direction);
// float c  = dot(oc, oc) - self.radius * self.radius;

// // Calculate the discriminant to determine if there's an intersection
// float discriminant = b * b - 4.0 * a * c;

// if (discriminant > 0.0)
// {
//     // Ray intersects the sphere
//     // Calculate the value of t for the two possible intersection points
//     float t1 = (-b - sqrt(discriminant)) / (2.0 * a);
//     float t2 = (-b + sqrt(discriminant)) / (2.0 * a);

//     // Choose the smaller positive t value
//     float t = min(t1, t2);

//     // Calculate the intersection point
//     vec3 intersectionPoint = Ray_at(ray, t);

//     if (dot(intersectionPoint - ray.origin, ray.direction) > 0.0)
//     {
//         return IntersectionResult_getMiss();
//     }

//     // Calculate and return the color based on the intersection point
//     IntersectionResult result;
//     result.intersection_occured = true;
//     result.maybe_distance       = length(ray.origin - intersectionPoint);
//     result.maybe_normal = normalize(intersectionPoint - self.center);
//     result.maybe_color  = vec4(1.0, 1.0, 1.0, 1.0);

//     return result;
// }
// else
// {
//     return IntersectionResult_getMiss();
// }