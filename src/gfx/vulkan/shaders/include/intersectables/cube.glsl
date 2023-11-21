#ifndef SRC_GFX_VULKAN_SHADERS_INCLUDE_INTERSECTABLES_CUBE_GLSL
#define SRC_GFX_VULKAN_SHADERS_INCLUDE_INTERSECTABLES_CUBE_GLSL

#include <intersectable.glsl>
#include <ray.glsl>
#include <util.glsl>

struct Cube
{
    vec3  center;
    float edge_length;
};

IntersectionResult Cube_tryIntersect(const Cube self, in Ray ray)
{
    ray.direction = -ray.direction; // FIXME: what, this is terrible

    float tmin, tmax, tymin, tymax, tzmin, tzmax;

    vec3 invdir = 1 / ray.direction;

    vec3 bounds[2] =
        vec3[2](self.center - self.edge_length, self.center + self.edge_length);

    tmin  = (bounds[int(invdir[0] < 0)].x - ray.origin.x) * invdir.x;
    tmax  = (bounds[1 - int(invdir[0] < 0)].x - ray.origin.x) * invdir.x;
    tymin = (bounds[int(invdir[1] < 0)].y - ray.origin.y) * invdir.y;
    tymax = (bounds[1 - int(invdir[1] < 0)].y - ray.origin.y) * invdir.y;

    if ((tmin > tymax) || (tymin > tmax))
    {
        return IntersectionResult_getMiss();
    }

    if (tymin > tmin)
    {
        tmin = tymin;
    }
    if (tymax < tmax)
    {
        tmax = tymax;
    }

    tzmin = (bounds[int(invdir[2] < 0)].z - ray.origin.z) * invdir.z;
    tzmax = (bounds[1 - int(invdir[2] < 0)].z - ray.origin.z) * invdir.z;

    if ((tmin > tzmax) || (tzmin > tmax))
    {
        return IntersectionResult_getMiss();
    }

    if (tzmin > tmin)
    {
        tmin = tzmin;
    }
    if (tzmax < tmax)
    {
        tmax = tzmax;
    }

    if (tmin < 0.0)
    {
        // The intersection point is behind the ray's origin, consider it a miss
        return IntersectionResult_getMiss();
    }

    IntersectionResult result;
    result.intersection_occured = true;

    // Calculate the normal vector based on which face is hit
    vec3 hit_point = ray.origin + tmin * ray.direction; // TODO: inverse?

    result.maybe_distance = length(ray.origin - hit_point);
    vec3 normal;

    if (isApproxEqual(hit_point.x, bounds[1].x))
    {
        normal = vec3(-1.0, 0.0, 0.0); // Hit right face
    }
    else if (isApproxEqual(hit_point.x, bounds[0].x))
    {
        normal = vec3(1.0, 0.0, 0.0); // Hit left face
    }
    else if (isApproxEqual(hit_point.y, bounds[1].y))
    {
        normal = vec3(0.0, -1.0, 0.0); // Hit top face
    }
    else if (isApproxEqual(hit_point.y, bounds[0].y))
    {
        normal = vec3(0.0, 1.0, 0.0); // Hit bottom face
    }
    else if (isApproxEqual(hit_point.z, bounds[1].z))
    {
        normal = vec3(0.0, 0.0, -1.0); // Hit front face
    }
    else if (isApproxEqual(hit_point.z, bounds[0].z))
    {
        normal = vec3(0.0, 0.0, 1.0); // Hit back face
    }

    result.maybe_normal = normal;
    result.maybe_color  = vec4(0.0, 1.0, 1.0, 1.0);

    return result;
}

#endif // SRC_GFX_VULKAN_SHADERS_INCLUDE_INTERSECTABLES_CUBE_GLSL
