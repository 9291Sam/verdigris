#version 460
#extension GL_EXT_shader_explicit_arithmetic_types : enable

#include <ray.glsl>

layout(location = 0) in vec3 in_world_intersection_position;
layout(location = 1) in flat uint32_t in_brick_pointer;

layout(push_constant) uniform PushConstants
{
    mat4 model_view_proj;
    mat4 model;
    vec3 camera_pos;
}
in_push_constants;

layout(location = 0) out vec4 out_color;

void main()
{
    out_color =
        vec4(in_world_intersection_position - in_push_constants.camera_pos, 1);
    // Ray ray;
    // ray.origin    = in_push_constants.camera_pos;
    // ray.direction = normalize(
    //     in_world_intersection_position - in_push_constants.camera_pos);

    // if (in_brick_pointer == ~0)
    // {
    //     out_color = vec4(ray.direction, 1);
    // }
    // else
    // {
    //     out_color = vec4(1);
    // }
}