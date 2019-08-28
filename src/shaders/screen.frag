#version 410
precision highp float;

in vec3 f_camPosWorldSpace;
in vec3 f_posCamSpace;

out vec4 out_color;

uniform vec2 u_windowSize;

uniform sampler2D u_framebuffer;
uniform sampler2D u_counter;

void main() {
	vec2 uv = gl_FragCoord.xy / u_windowSize;
	vec3 rgb = texture(u_framebuffer, uv).rgb;
	float count = texture(u_counter, uv).x;

	vec3 L = rgb / count;
    L.x = pow(clamp(L.x, 0.0, 1.0), 1.0 / 2.2);
    L.y = pow(clamp(L.y, 0.0, 1.0), 1.0 / 2.2);
    L.z = pow(clamp(L.z, 0.0, 1.0), 1.0 / 2.2);
    out_color = vec4(L, 1.0);
}
