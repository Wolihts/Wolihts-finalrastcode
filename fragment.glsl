#version 330

in vec3 v_normal;
in vec2 v_uv;
uniform float time;
out vec4 frag_color;

void main() {
    vec3 ld = normalize(vec3(1.0,1.0,1.0));
    float df = max(dot(normalize(v_normal),ld),0.0);
    float am = 0.25;
    float p = sin(time + v_uv.x * 12.0 + v_uv.y * 12.0) * 0.5 + 0.5;
    vec3 blue = vec3(0.05,0.25,0.95);
    vec3 glow = vec3(0.20,0.65,1.0) * p * 0.35;
    vec3 col = blue * (am + df) + glow;

    frag_color = vec4(col,1.0);
}