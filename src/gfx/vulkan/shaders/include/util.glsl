#ifndef SRC_GFX_VULKAN_SHADERS_INCLUDE_UTIL_GLSL
#define SRC_GFX_VULKAN_SHADERS_INCLUDE_UTIL_GLSL

vec4 convertSRGBToLinear(vec4 srgbColor)
{
    bvec3 cutoff = lessThan(srgbColor.rgb, vec3(0.04045));
    vec3 higher = pow((srgbColor.rgb + vec3(0.055)) / vec3(1.055), vec3(2.4));
    vec3 lower = srgbColor.rgb / vec3(12.92);

    return vec4(mix(higher, lower, cutoff), srgbColor.a);
}

vec4 convertLinearColorToSRGB(vec4 linearColor)
{
    bvec3 cutoff = lessThan(linearColor.rgb, vec3(0.0031308));
    vec3 higher = vec3(1.055) * pow(linearColor.rgb, vec3(1.0 / 2.4)) - vec3(0.055);
    vec3 lower = linearColor.rgb * vec3(12.92);

    return vec4(mix(higher, lower, cutoff), linearColor.a);
}

#endif // SRC_GFX_VULKAN_SHADERS_INCLUDE_UTIL_GLSL