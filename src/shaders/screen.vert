#version 410
precision highp float;

vec2 positions[] = vec2[](
    vec2(-1.0, -1.0),
    vec2(-1.0,  1.0),
    vec2( 1.0, -1.0),
    vec2( 1.0,  1.0)
);

uint indices[] = uint[](
    0u, 1u, 3u, 0u, 3u, 2u
);

void main() {
	gl_Position = vec4(positions[indices[gl_VertexID]], 0.0, 1.0);
}
