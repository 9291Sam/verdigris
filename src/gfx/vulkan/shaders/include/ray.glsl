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
    float maybe_distance;
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

bool isEqual(float a, float b, float epsilon) {
    return abs(a - b) < epsilon;
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
        float t = max(t1, t2); // TODO: what, why is this the max???

        // Calculate the intersection point
        vec3 intersectionPoint = Ray_at(ray, t);

        if (dot(intersectionPoint - ray.origin, ray.direction) > 0.0)
        {
            return IntersectionResult_getMiss();
        }

        // Calculate and return the color based on the intersection point
        IntersectionResult result;
        result.intersection_occured = true;
        result.maybe_distance = length(ray.origin - intersectionPoint);
        result.maybe_normal = normalize(intersectionPoint - self.center);
        
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
    ray.direction = -ray.direction; // FIXME: what, this is terrible

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

    if (tmin < 0.0) {
        // The intersection point is behind the ray's origin, consider it a miss
        return IntersectionResult_getMiss();
    }


    IntersectionResult result;
    result.intersection_occured = true;

    // Calculate the normal vector based on which face is hit
    vec3 hit_point = ray.origin + tmin * ray.direction; // TODO: inverse?

    result.maybe_distance = length(ray.origin - hit_point);
    vec3 normal;
    
    const float EPSILON = 0.001;

    if (isEqual(hit_point.x, bounds[1].x, EPSILON))
        normal = vec3(-1.0, 0.0, 0.0); // Hit right face
    else if (isEqual(hit_point.x, bounds[0].x, EPSILON))
        normal = vec3(1.0, 0.0, 0.0); // Hit left face
    else if (isEqual(hit_point.y, bounds[1].y, EPSILON))
        normal = vec3(0.0, -1.0, 0.0); // Hit top face
    else if (isEqual(hit_point.y, bounds[0].y, EPSILON))
        normal = vec3(0.0, 1.0, 0.0); // Hit bottom face
    else if (isEqual(hit_point.z, bounds[1].z, EPSILON))
        normal = vec3(0.0, 0.0, -1.0); // Hit front face
    else if (isEqual(hit_point.z, bounds[0].z, EPSILON))
        normal = vec3(0.0, 0.0, 1.0); // Hit back face

    result.maybe_normal = normal;

    return result;
}




#endif // SRC_GFX_VULKAN_SHADERS_INCLUDE_RAY_GLSL