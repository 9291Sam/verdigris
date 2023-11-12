#ifndef SRC_GFX_VULKAN_SHADERS_INCLUDE_RAY_GLSL
#define SRC_GFX_VULKAN_SHADERS_INCLUDE_RAY_GLSL

struct Ray
{
    vec3 origin;
    vec3 direction;
};

vec3 Ray_at(in Ray self, in float t)
{
    return self.origin + self.direction * t;
}


struct IntersectionResult
{
    bool intersection_occured;
    vec3 maybe_normal;
};

IntersectionResult IntersectionResult_getMiss()
{
    IntersectionResult result;
    result.intersection_occured = false;

    return result;
}

struct Sphere
{
    vec3 center;
    float radius;
};

IntersectionResult Sphere_tryIntersect(in Sphere self, in Ray ray)
{

    vec3 oc = ray.origin - self.center;
    float a = dot(ray.direction, ray.direction);
    float b = 2.0 * dot(oc, ray.direction);
    float c = dot(oc, oc) - self.radius * self.radius;
    
    // Calculate the discriminant to determine if there's an intersection
    float discriminant = b * b - 4.0 * a * c;

    if (discriminant > 0.0) {
        // Ray intersects the sphere
        // Calculate the value of t for the two possible intersection points
        float t1 = (-b - sqrt(discriminant)) / (2.0 * a);
        float t2 = (-b + sqrt(discriminant)) / (2.0 * a);

        // Choose the smaller positive t value
        float t = min(t1, t2);

        // Calculate the intersection point
        vec3 intersectionPoint = Ray_at(ray, t);

        if (dot(intersectionPoint - ray.origin, ray.direction) > 0.0)
        {
            return IntersectionResult_getMiss();
        }

        // Calculate and return the color based on the intersection point
        vec3 normal = normalize(intersectionPoint - self.center);

        IntersectionResult result;
        result.intersection_occured = true;
        result.maybe_normal = normal;
        
        return result;
    }
    else
    {
        return IntersectionResult_getMiss();
    }
}

struct Cube
{
    vec3 center;
    float edge_length;
};

IntersectionResult Cube_tryIntersect(in Cube self, in Ray ray)
{
    float tmin, tmax, tymin, tymax, tzmin, tzmax;

    vec3 invdir = 1 / ray.direction;

    vec3 bounds[2] = vec3[2](self.center - self.edge_length, self.center + self.edge_length);
    
    tmin = (bounds[int(invdir[0] < 0)].x - ray.origin.x) * invdir.x;
    tmax = (bounds[1-int(invdir[0] < 0)].x - ray.origin.x) * invdir.x;
    tymin = (bounds[int(invdir[1] < 0)].y - ray.origin.y) * invdir.y;
    tymax = (bounds[1-int(invdir[1] < 0)].y - ray.origin.y) * invdir.y;
    
    if ((tmin > tymax) || (tymin > tmax))
        return IntersectionResult_getMiss();

    if (tymin > tmin)
        tmin = tymin;
    if (tymax < tmax)
        tmax = tymax;
    
    tzmin = (bounds[int(invdir[2] < 0)].z - ray.origin.z) * invdir.z;
    tzmax = (bounds[1-int(invdir[2] < 0)].z - ray.origin.z) * invdir.z;
    
    if ((tmin > tzmax) || (tzmin > tmax))
        return IntersectionResult_getMiss();

    if (tzmin > tmin)
        tmin = tzmin;
    if (tzmax < tmax)
        tmax = tzmax;

    IntersectionResult result;
    result.intersection_occured = true;
    result.maybe_normal = vec3(1.0, 1.0, 1.0);

    return result;
}




#endif // SRC_GFX_VULKAN_SHADERS_INCLUDE_RAY_GLSL