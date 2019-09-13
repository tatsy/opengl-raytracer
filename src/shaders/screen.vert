#version 450
#extension GL_ARB_separate_shader_objects : enable

precision highp float;

vec2 positions[4] = vec2[](
    vec2(-1.0, -1.0),
    vec2(-1.0,  1.0),
    vec2( 1.0, -1.0),
    vec2( 1.0,  1.0)
);

uint indices[6] = uint[](
    0u, 1u, 3u, 0u, 3u, 2u
);

void main() {
	gl_Position = vec4(positions[indices[gl_VertexIndex]], 0.0, 1.0);
}
