#version 410
precision highp float;
//#extension GL_ARB_gpu_shader_fp64 : enable
//#define float double
//#define vec2 dvec2
//#define vec3 dvec3
//#define vec4 dvec4

in vec3 f_camPosWorldSpace;
in vec3 f_posCamSpace;

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec4 out_count;

uniform vec2 u_seed;
uniform int u_maxDepth = 8;
uniform int u_nSamples = 8;
uniform vec2 u_windowSize;
uniform mat4 u_mvMat;

uniform sampler2D u_framebuffer;
uniform sampler2D u_counter;

uniform int u_nTris;
uniform samplerBuffer u_vertBuffer;
uniform samplerBuffer u_triBuffer;
uniform samplerBuffer u_matBuffer;

uniform int u_nLights;
uniform samplerBuffer u_lightBuffer;

#define PI 3.14159265358979
#define INFTY 1.0e12
#define EPS 1.0e-2

// ----------------------------------------------------------------------------
// Structs
// ----------------------------------------------------------------------------

struct Ray {
    vec3 o;
    vec3 d;
};

struct Triangle {
	vec3 v[3];
};

// ----------------------------------------------------------------------------
// Random number generator
// ----------------------------------------------------------------------------

vec2 randState;

float rand() {
    float a = 12.9898;
    float b = 78.233;
    float c = 43758.5453;
    randState.x = fract(sin(dot(randState.xy - u_seed, vec2(a, b))) * c);
    randState.y = fract(sin(dot(randState.xy - u_seed, vec2(a, b))) * c);
    return randState.x;
}

// ----------------------------------------------------------------------------
// Intersection test
// ----------------------------------------------------------------------------

float intersect(in Ray ray, in Triangle tri, out vec3 norm) {
	vec3 e1 = tri.v[1] - tri.v[0];
	vec3 e2 = tri.v[2] - tri.v[0];
	vec3 pVec = cross(ray.d, e2);

	float det = dot(e1, pVec);
	if (det > -EPS && det < EPS) {
		return INFTY;
	}

	float invdet = 1.0 / det;
	vec3 tVec = ray.o - tri.v[0];
	float u = dot(tVec, pVec) * invdet;
	if (u < 0.0 || u > 1.0) {
		return INFTY;
	}

	vec3 qVec = cross(tVec, e1);
	float v = dot(ray.d, qVec) * invdet;
	if (v < 0.0 || u + v > 1.0) {
		return INFTY;
	}

	float t = dot(e2, qVec) * invdet;
	if (t <= EPS) {
		return INFTY;
	}

	norm = normalize(cross(normalize(e1), normalize(e2)));

	return t;
}

bool intersect(in Ray ray, out float t, out vec3 norm, out int mtrlID) {
	t = INFTY;
	norm = vec3(0.0);
	bool hit = false;
    for (int i = 0; i < u_nTris; i++) {
		vec3 ijk = texelFetch(u_triBuffer, i).xyz;

		Triangle tri;
		tri.v[0] = texelFetch(u_vertBuffer, int(ijk.x) * 5 + 0).xyz;
		tri.v[1] = texelFetch(u_vertBuffer, int(ijk.y) * 5 + 0).xyz;
		tri.v[2] = texelFetch(u_vertBuffer, int(ijk.z) * 5 + 0).xyz;

		vec3 nn;
        float dist = intersect(ray, tri, nn);
        if (dist < t) {
            t = dist;
			mtrlID = int(texelFetch(u_triBuffer, i).w);
			norm = nn;
			hit = true;
        }
    }

	return hit;
}

vec3 sampleDirect(in vec3 x, in vec3 n) {
	int lightID = min(int(rand() * u_nLights), u_nLights - 1);

	vec2 u = vec2(rand(), rand());
	if (u.x + u.y > 1.0) {
		u.x = 1.0 - u.x;
		u.y = 1.0 - u.y;
	}

	vec3 ijk = texelFetch(u_lightBuffer, lightID).xyz;

	Triangle tri;
	tri.v[0] = texelFetch(u_vertBuffer, int(ijk.x) * 5 + 0).xyz;
	tri.v[1] = texelFetch(u_vertBuffer, int(ijk.y) * 5 + 0).xyz;
	tri.v[2] = texelFetch(u_vertBuffer, int(ijk.z) * 5 + 0).xyz;

	vec3 p = (1.0 - u.x - u.y) * tri.v[0] + u.x * tri.v[1] + u.y * tri.v[2];

	vec3 dir = normalize(p - x);
	Ray ray = Ray(x, dir);

	float tHit;
	vec3 norm;
	int id;
	bool isHit = intersect(ray, tHit, norm, id);

	float dist = length(p - x);
	if (isHit && dist < tHit + EPS) {
		int mtrlID = int(texelFetch(u_lightBuffer, lightID).w);
		vec3 e = texelFetch(u_matBuffer, mtrlID * 2 + 0).xyz;
		float dot0 = max(0.0, dot(ray.d, n));
		float dot1 = max(0.0, dot(-ray.d, norm));
		float G = (dot0 * dot1) / (dist * dist);
		float area = 0.5 * length(cross(tri.v[1] - tri.v[0], tri.v[2] - tri.v[0]));
		float pdf = 1.0 / (area * float(u_nLights));
		return e * G / pdf;
	}
	return vec3(0.0);
}

vec3 radiance(in Ray ray){
    vec3 L = vec3(0.0);
    vec3 beta = vec3(1.0);
    Ray r = ray;

    for (int depth = 0; depth < 2; depth++) {
        float t = INFTY;
		vec3 norm;
        int id = 0;
		bool isIntersect = intersect(r, t, norm, id);

		vec3 x = r.o + r.d * t;
		vec3 e = texelFetch(u_matBuffer, id * 2 + 0).xyz;
        vec3 f = texelFetch(u_matBuffer, id * 2 + 1).xyz;

		if (depth == 0) {
			if (isIntersect) {
				L += beta * e;
			}
		}

		if (!isIntersect) {
			break;
		}

		// Next event estimation
		if (depth >= 0) {
			vec3 Ld = sampleDirect(x, norm);
			L += f * beta * Ld;
		}

		if (length(f) == 0.0) {
			break;
		}
		
		// Sample BRDF
        float r1 = 2.0 * PI * rand();
        float r2 = rand();
        float r2s = sqrt(r2);
        vec3 w = norm;
        vec3 u = cross(abs(w.x) > 0.1 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0), w);
        vec3 v = cross(w, u);
        vec3 d = normalize(u * cos(r1) * r2s + v * sin(r1) * r2s + w * sqrt(1.0 - r2));
        r = Ray(x, d);

		// Update beta
        beta *= f;

        // Russian roulette
        if (depth > 3) {
	        float p = min(0.95, max(beta.x, max(beta.y, beta.z)));
            if (rand() > p) {
				break;
			}
            beta /= p;
        }
    }

    return L;
}

void main(void) {
    randState = gl_FragCoord.xy / u_windowSize;

    Ray ray = Ray(
        f_camPosWorldSpace,
        normalize((inverse(u_mvMat) * vec4(f_posCamSpace, 0.0)).xyz)
    );

	vec2 uv = gl_FragCoord.xy / u_windowSize;
	vec3 L = texture(u_framebuffer, uv).rgb;
	float count = texture(u_counter, uv).x;
    for (int i = 0; i < u_nSamples; i++) {
        L += radiance(ray);
		count += 1.0;
    }

    out_color = vec4(L, 1.0);
	out_count = vec4(count, count, count, 1.0);
}
