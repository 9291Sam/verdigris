#version 460

layout(std140, set = 0, binding = 0) readonly buffer VoxelPositions
{
    vec3 positions[];
}
in_positions;

layout(std140, set = 0, binding = 1) readonly buffer VoxelSizes
{
    float sizes[];
}
in_sizes;

layout(std140, set = 0, binding = 2) readonly buffer VoxelColors
{
    vec4 color[];
}
in_colors;

layout(push_constant) uniform PushConstants
{
    mat4 model_view_proj;
}
in_push_constants;

layout(location = 0) out vec4 out_color;

const vec3 CUBE_STRIP_OFFSETS[] = {
    vec3(-0.5f, 0.5f, 0.5f),   // Front-top-left
    vec3(0.5f, 0.5f, 0.5f),    // Front-top-right
    vec3(-0.5f, -0.5f, 0.5f),  // Front-bottom-left
    vec3(0.5f, -0.5f, 0.5f),   // Front-bottom-right
    vec3(0.5f, -0.5f, -0.5f),  // Back-bottom-right
    vec3(0.5f, 0.5f, 0.5f),    // Front-top-right
    vec3(0.5f, 0.5f, -0.5f),   // Back-top-right
    vec3(-0.5f, 0.5f, 0.5f),   // Front-top-left
    vec3(-0.5f, 0.5f, -0.5f),  // Back-top-left
    vec3(-0.5f, -0.5f, 0.5f),  // Front-bottom-left
    vec3(-0.5f, -0.5f, -0.5f), // Back-bottom-left
    vec3(0.5f, -0.5f, -0.5f),  // Back-bottom-right
    vec3(-0.5f, 0.5f, -0.5f),  // Back-top-left
    vec3(0.5f, 0.5f, -0.5f),   // Back-top-right
};

void main()
{
    const vec3  center_location = in_positions.positions[gl_VertexIndex / 14];
    const float voxel_size      = in_sizes.sizes[gl_VertexIndex / 14];
    const vec4  voxel_color     = in_colors.color[gl_VertexIndex / 14];

    const vec3 cube_vertex    = CUBE_STRIP_OFFSETS[gl_VertexIndex % 14];
    const vec3 world_location = center_location + voxel_size * cube_vertex;

    gl_Position = in_push_constants.model_view_proj * vec4(world_location, 1.0);
    out_color   = voxel_color;
}