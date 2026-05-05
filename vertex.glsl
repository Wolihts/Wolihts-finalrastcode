#version 330

in vec3 position;
in vec3 normal;
in vec2 uv;
uniform mat4 model;
uniform mat4 view_proj;
out vec3 v_normal;
out vec2 v_uv;

void main() {
    vec4 world_pos = model * vec4(position,1.0);
    gl_Position = view_proj * world_pos;

    v_normal = normalize(mat3(model) * normal);
    v_uv = uv;
}