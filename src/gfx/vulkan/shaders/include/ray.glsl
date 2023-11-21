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

#endif // SRC_GFX_VULKAN_SHADERS_INCLUDE_RAY_GLSL