#version 450
#extension GL_ARB_separate_shader_objects : enable

precision highp float;

layout(binding = 0) uniform UniformBufferObject {
    uniform mat4 u_c2wMat;
    uniform mat4 u_s2cMat;
} ubo;

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
	gl_Position = vec4(positions[indices[gl_VertexIndex]], 0.0, 1.0);
}
