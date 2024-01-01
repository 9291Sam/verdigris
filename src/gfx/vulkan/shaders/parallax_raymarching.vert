#version 460
#extension GL_EXT_shader_explicit_arithmetic_types : enable

layout(location = 0) in vec3 in_position;
layout(location = 1) in uint32_t in_brick_pointer;

layout(push_constant) uniform PushConstants
{
    mat4 model_view_proj;
    mat4 model;
    vec3 camera_pos;
}
in_push_constants;

layout(location = 0) out vec3 out_world_intersection_position;
layout(location = 1) out flat uint32_t out_brick_pointer;

void main()
{
    gl_Position = in_push_constants.model_view_proj * vec4(in_position, 1.0);

    vec4 intercalc = in_push_constants.model * vec4(in_position, 1.0);
    intercalc /= intercalc.w;

    out_world_intersection_position = intercalc.xyz;
    out_brick_pointer               = in_brick_pointer;
}