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

struct Sphere
{
    vec3 center;
    float radius;
};

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



#endif // SRC_GFX_VULKAN_SHADERS_INCLUDE_RAY_GLSL