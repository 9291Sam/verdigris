#ifndef SRC_GFX_VULKAN_SHADERS_INCLUDE_UTIL_GLSL
#define SRC_GFX_VULKAN_SHADERS_INCLUDE_UTIL_GLSL

vec4 convertSRGBToLinear(const in vec4 srgbColor)
{
    const bvec3 cutoff = lessThan(srgbColor.rgb, vec3(0.04045));
    const vec3  higher =
        pow((srgbColor.rgb + vec3(0.055)) / vec3(1.055), vec3(2.4));
    const vec3 lower = srgbColor.rgb / vec3(12.92);

    return vec4(mix(higher, lower, cutoff), srgbColor.a);
}

vec4 convertLinearColorToSRGB(const in vec4 linearColor)
{
    const bvec3 cutoff = lessThan(linearColor.rgb, vec3(0.0031308));
    const vec3  higher =
        vec3(1.055) * pow(linearColor.rgb, vec3(1.0 / 2.4)) - vec3(0.055);
    const vec3 lower = linearColor.rgb * vec3(12.92);

    return vec4(mix(higher, lower, cutoff), linearColor.a);
}

float map(
    const in float x,
    const in float in_min,
    const in float in_max,
    const in float out_min,
    const in float out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

#define VERDIGRIS_EPSILON 0.001

bool isApproxEqual(const in float a, const in float b)
{
    return abs(a - b) < VERDIGRIS_EPSILON;
}

float maxComponent(const vec3 vec)
{
    return max(vec.x, max(vec.y, vec.z));
}

float minComponent(const vec3 vec)
{
    return min(vec.x, min(vec.y, vec.z));
}

#endif // SRC_GFX_VULKAN_SHADERS_INCLUDE_UTIL_GLSL