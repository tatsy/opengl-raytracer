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
uniform int u_nSamples = 1;
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

uniform float u_densityMax = 1.0;
uniform vec3 u_bboxMin = vec3(0.0);
uniform vec3 u_bboxMax = vec3(1.0);
uniform sampler3D u_densityTex;
uniform sampler3D u_temperatureTex;

const float PI = 3.14159265358979;
const float INFTY = 1.0e5;
const float EPS = 1.0e-3;

// ----------------------------------------------------------------------------
// Structs
// ----------------------------------------------------------------------------

struct Ray {
    vec3 o;
    vec3 d;
};

struct Triangle {
	vec3 v[3];
    vec3 n[3];
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
// Utility methods
// ----------------------------------------------------------------------------

bool isBlack(in vec3 x) {
	return length(x) == 0.0;
}

float bbr(float l, float T) {
	float h = 6.6260e-34;
	float c = 2.9979e8;
	float k = 1.3806e-23;
	float l5 = (l * l) * (l * l) * l;
	return (2 * h * c * c) / (l5 * (exp((h * c) / (l * k  * T)) - 1));
}

vec3 blackBody(float T) {
	float lambdaR = 6.10e-7;
	float lambdaG = 5.50e-7;
	float lambdaB = 4.50e-7;

    float R = max(0.0, bbr(lambdaR, T));
    float G = max(0.0, bbr(lambdaG, T));
    float B = max(0.0, bbr(lambdaB, T));
	return vec3(R, G, B);
}

float densityLookup(in vec3 pos) {
    vec3 uvw = (pos - u_bboxMin) / (u_bboxMax - u_bboxMin);
    return texture(u_densityTex, uvw).x;
}

float temperatureLookup(in vec3 pos) {
    vec3 uvw = (pos - u_bboxMin) / (u_bboxMax - u_bboxMin);
    return texture(u_temperatureTex, uvw).x;
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

	norm = (1.0 - u - v) * tri.n[0] + u * tri.n[1] + v * tri.n[2];

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
        tri.n[0] = texelFetch(u_vertBuffer, int(ijk.x) * 5 + 1).xyz;
        tri.n[1] = texelFetch(u_vertBuffer, int(ijk.y) * 5 + 1).xyz;
        tri.n[2] = texelFetch(u_vertBuffer, int(ijk.z) * 5 + 1).xyz;

		vec3 nn;
        float dist = intersect(ray, tri, nn);
        if (dist < t) {
            t = dist;
			norm = nn;
            mtrlID = int(texelFetch(u_triBuffer, i).w);
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
		vec3 e = texelFetch(u_matBuffer, mtrlID * 3 + 0).xyz;
		float dot0 = max(0.0, dot(ray.d, n));
		float dot1 = max(0.0, dot(-ray.d, norm));
		float G = (dot0 * dot1) / (dist * dist);
		float area = 0.5 * length(cross(tri.v[1] - tri.v[0], tri.v[2] - tri.v[0]));
		float pdf = 1.0 / (area * float(u_nLights));
		return e * G / pdf;
	}
	return vec3(0.0);
}

vec3 sampleDirectVolume(in vec3 x) {
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
    if (isHit && abs(dist - tHit) < EPS) {
        int mtrlID = int(texelFetch(u_lightBuffer, lightID).w);
        vec3 e = texelFetch(u_matBuffer, mtrlID * 3 + 0).xyz;
        float dot1 = max(0.0, dot(-ray.d, norm));
        float G = dot1 / (dist * dist);
        float area = 0.5 * length(cross(tri.v[1] - tri.v[0], tri.v[2] - tri.v[0]));
        float pdf = 1.0 / (area * float(u_nLights));
        return e * G / pdf / (4.0 * PI);
    }
    return vec3(0.0);
}

Ray spawnRay(in vec3 org, in vec3 dir) {
    return Ray(org + dir * EPS, dir);
}

vec3 radiance(in Ray r){
    vec3 L = vec3(0.0);
    vec3 beta = vec3(1.0);
    Ray ray = r;
    bool passedVolume = false;

    for (int depth = 0; depth < u_maxDepth; depth++) {
        float tHit = INFTY;
		vec3 norm;
        int id = 0;
		bool isIntersect = intersect(ray, tHit, norm, id);

		vec3 x = ray.o + tHit * ray.d;
		vec3 e = texelFetch(u_matBuffer, id * 3 + 0).xyz;
        vec3 f = texelFetch(u_matBuffer, id * 3 + 1).xyz;

		if (isBlack(e) && isBlack(f)) {
			// Volume (perform Woodcock tracking)
            vec3 sigS = vec3(1.0, 1.0, 1.0) * 0.1;
            vec3 sigA = vec3(0.1, 0.1, 0.1) * 0.1;
            float sigT = sigS.x + sigA.x;

			int nTrial = 16;
			vec3 nextOrg = x;
			vec3 nextDir = ray.d;
			for (int k = 0; k < nTrial; k++) {
				Ray nextRay = spawnRay(nextOrg, nextDir);
				float tMax;
				vec3 unused0;
				int unused1;
				bool hit = intersect(nextRay, tMax, unused0, unused1);
                if (!hit) {
                    return vec3(0, 0, 0);
                }

				float t = 0.0;
                bool pass = false;
				for (int i = 0; i < 128; i++) {
					t -= log(max(EPS, 1.0 - rand())) / (u_densityMax * sigT);
					if (t >= tMax) {
                        pass = true;
						break;
					}

					vec3 pos = nextOrg + t * nextDir;
					float density = densityLookup(pos);
					if (density / u_densityMax > rand()) {
                        pass = true;
						break;
					}
				}

                if (!pass) {
                    return vec3(0.0, 0.0, 0.0);
                }

				if (t > tMax) {
                    nextOrg = nextOrg + tMax * nextDir;
                    break;
				}

				nextOrg = nextOrg + t * nextDir;

                // Black body radiation
				float T = temperatureLookup(nextOrg) * 45.0;
				L += beta * blackBody(T) / sigT;

                // Direct lighting
                L += beta * (sigS / sigT) * sampleDirectVolume(nextOrg);

				// Sample path direction
				float theta = acos(2.0 * rand() - 1.0);
				float phi = 2.0 * PI * rand();
				float cosTheta = cos(theta);
				float sinTheta = sin(theta);
				nextDir = vec3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);

                // Scattering albedo
				beta *= sigS / sigT;
			}

            ray = spawnRay(nextOrg, nextDir);
            passedVolume = true;
			continue;
		}

		if (depth == 0 || passedVolume) {
			if (isIntersect) {
				L += beta * e;
			}
		}

		if (!isIntersect) {
			break;
		}

		// Direct lighting
        L += f * beta * sampleDirect(x, norm);

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
        ray = spawnRay(x, d);

		// Update beta
        beta *= f;
        passedVolume = false;

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
