#version 410
#extension GL_ARB_gpu_shader_fp64 : enable

precision highp float;

uniform mat4 u_c2wMat;
uniform mat4 u_s2cMat;

vec2 positions[] = vec2[](
    vec2(-1.0, -1.0),
    vec2(-1.0,  1.0),
    vec2( 1.0, -1.0),
    vec2( 1.0,  1.0)
);

uint indices[] = uint[](
    0u, 1u, 3u, 0u, 3u, 2u
);

void main(void) {
	gl_Position = vec4(positions[indices[gl_VertexID]], 0.0, 1.0);
}
