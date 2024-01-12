#version 460

layout(location = 0) in vec4 in_color;
layout(location = 1) in vec3 in_position;
layout(location = 2) in vec3 in_normal;

layout(push_constant) uniform PushConstants
{
    mat4 model_view_proj;
}
in_push_constants;

layout(location = 0) out vec4 out_color;

void main()
{
    gl_Position = in_push_constants.model_view_proj * vec4(in_position, 1.0);
    out_color   = in_color;
    // out_normal  = in_normal;
}