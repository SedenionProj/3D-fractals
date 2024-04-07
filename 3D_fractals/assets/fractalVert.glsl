#version 460

vec2 positions[6] = vec2[](
    vec2(-1, -1),
    vec2(-1,  1),
    vec2( 1, -1),
    vec2( 1,  1),
    vec2(-1,  1),
    vec2( 1, -1)
);

void main() {
    gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);
}