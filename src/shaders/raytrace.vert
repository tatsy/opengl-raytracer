#version 410
precision highp float;
//#extension GL_ARB_gpu_shader_fp64 : enable
//#define float double
//#define vec2 dvec2
//#define vec3 dvec3
//#define vec4 dvec4

out vec3 f_camPosWorldSpace;
out vec3 f_posCamSpace;

uniform mat4 u_mvMat;
uniform mat4 u_projMat;

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

	vec4 temp;
	temp = inverse(u_mvMat) * vec4(0.0, 0.0, 0.0, 1.0);
	f_camPosWorldSpace = temp.xyz / temp.w;
	temp = inverse(u_projMat) * gl_Position;
	f_posCamSpace = temp.xyz / temp.w;
}
