#version 450
#extension GL_ARB_separate_shader_objects : require

precision highp float;

layout(location = 0) out vec4 out_color;

layout(binding = 0) uniform UniformBufferObject {
    float gamma;
    vec2 windowSize;
} ubo;

layout(binding = 1) uniform sampler2D u_framebuffer;
layout(binding = 2) uniform sampler2D u_counter;

void main() {
	vec2 uv = gl_FragCoord.xy / ubo.windowSize;
	vec3 rgb = texture(u_framebuffer, uv).rgb;
	float count = texture(u_counter, uv).x;

	vec3 L = rgb / count;
    L.x = pow(clamp(L.x, 0.0, 1.0), 1.0 / ubo.gamma);
    L.y = pow(clamp(L.y, 0.0, 1.0), 1.0 / ubo.gamma);
    L.z = pow(clamp(L.z, 0.0, 1.0), 1.0 / ubo.gamma);
    out_color = vec4(L, 1.0);
}
