#version 410
precision highp float;

in vec3 f_camPosWorldSpace;
in vec3 f_posCamSpace;

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec4 out_count;

uniform float u_seed;
uniform int u_maxDepth = 8;
uniform int u_nSamples = 8;
uniform vec2 u_windowSize;
uniform mat4 u_mvMat;

uniform sampler2D u_framebuffer;
uniform sampler2D u_counter;

#define DIFF 1
#define SPEC 2
#define REFR 3

#define PI 3.14159265358979
#define INFTY 1.0e12

struct Ray {
    vec3 o;
    vec3 d;
};

struct Sphere {
    float r;
    vec3 p;
    vec3 e;
    vec3 c;
    int refl;
};

int nSphere = 9;
Sphere spheres[] = Sphere[]( //Scene: radius, position, emission, color, material
    Sphere(1e4, vec3( 1e4+1,40.8,81.6), vec3(0.0),vec3(.75,.25,.25),DIFF),//Left
    Sphere(1e4, vec3(-1e4+99,40.8,81.6),vec3(0.0),vec3(.25,.25,.75),DIFF),//Rght
    Sphere(1e4, vec3(50,40.8, 1e4),     vec3(0.0),vec3(.75,.75,.75),DIFF),//Back
    Sphere(1e4, vec3(50,40.8,-1e4+170), vec3(0.0),vec3(0.0),        DIFF),//Frnt
    Sphere(1e4, vec3(50, 1e4, 81.6),    vec3(0.0),vec3(.75,.75,.75),DIFF),//Botm
    Sphere(1e4, vec3(50,-1e4+81.6,81.6),vec3(0.0),vec3(.75,.75,.75),DIFF),//Top
    Sphere(16.5,vec3(27,16.5,47),       vec3(0.0),vec3(1,1,1)*.999, SPEC),//Mirr
    Sphere(16.5,vec3(73,16.5,78),       vec3(0.0),vec3(1,1,1)*.999, REFR),//Glas
    Sphere(600, vec3(50,681.6-.27,81.6),vec3(12,12,12),  vec3(0.0), DIFF) //Lite
);

float intersectSphere(Ray ray, Sphere sph) {
    vec3 op = sph.p - ray.o;
    float t;
    float eps = 1e-2;
    float b = dot(op, ray.d);
    float det = b * b - dot(op, op) + sph.r * sph.r;
    if (det < 0.0) {
        return INFTY;
    } else {
        det = sqrt(det);
    }

    t = b - det;
    if (t > eps) {
        return t;
    }

    t = b + det;
    if (t > eps) {
        return t;
    }

    return INFTY;
}

bool intersect(in Ray ray, out float t, out int id) {
    t = INFTY;
    id = 0;
    for (int i = 0; i < nSphere; i++) {
        float dist = intersectSphere(ray, spheres[i]);
        if (dist < t) {
            t = dist;
            id = i;
        }
    }

    return t != INFTY;
}

vec2 randState;

float rand() {
    float a = 12.9898;
    float b = 78.233;
    float c = 43758.5453;
    randState.x = fract(sin(dot(randState.xy + u_seed, vec2(a, b))) * c);
    randState.y = fract(sin(dot(randState.xy + u_seed, vec2(a, b))) * c);
    return randState.x;
}

vec3 radiance(in Ray ray){
    vec3 L = vec3(0.0);
    vec3 beta = vec3(1.0);
    Ray r = ray;

    for (int depth = 0; depth < u_maxDepth; depth++) {
        float t;
        int id = 0;
        if (!intersect(r, t, id)) {
            return vec3(0.0);
        }

        Sphere obj = spheres[id];
        vec3 x = r.o + r.d * t;
        vec3 n = normalize(x - obj.p);
        vec3 nl = dot(n, r.d) < 0.0 ? n : -n;
        vec3 f = obj.c;

        // Russian roulette
        float p = max(beta.x, max(beta.y, beta.z));
        if (depth > 2) {
            if (rand() < p) {
                beta /= p;
            } else {
                L += beta * obj.e;
                break;
            }
        }

        L += beta * obj.e;
        beta *= f;

        if (obj.refl == DIFF) {
            float r1 = 2.0 * PI * rand();
            float r2 = rand();
            float r2s = sqrt(r2);
            vec3 w = nl;
            vec3 u = cross(abs(w.x) > 0.1 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0), w);
            vec3 v = cross(w, u);
            vec3 d = normalize(u * cos(r1) * r2s + v * sin(r1) * r2s + w * sqrt(1.0 - r2));
            r = Ray(x, d);
        } else if (obj.refl == SPEC) {
            r = Ray(x, r.d - n * 2.0 * dot(n, r.d));
        } else if (obj.refl == REFR) {
			Ray reflRay = Ray(x, r.d - n * 2.0 * dot(n, r.d)); 
			bool into = dot(n, nl) > 0.0;
			float nc = 1.0;
			float nt = 1.5;
			float nnt = into ? nc / nt : nt / nc;
			float ddn = dot(r.d, nl);
			float cos2t = 1.0 - nnt * nnt * (1.0 - ddn * ddn);
			if (cos2t < 0.0) {
				// Total reflection
				r = reflRay;
			} else {
				vec3 tdir = normalize(r.d * nnt - n * ((into ? 1.0 : -1.0) * (ddn * nnt + sqrt(cos2t))));
				float a = nt - nc;
				float b = nt + nc;
				float R0 = (a * a) / (b * b);
				float c = 1.0 - (into ? -ddn : dot(tdir, n));
				float Re = R0 + (1.0 - R0) * (c * c * c * c * c);
				float P = 0.25 + 0.5 * Re;
				if (rand() < P) {
					// Reflection
					r = reflRay;
					beta *= Re / P;
				} else {
					// Refraction
					r = Ray(x, tdir);
					beta *= (1.0 - Re) / (1.0 - P);
				}
			}
		}
    }

    return L;
}

void main() {
    Ray ray = Ray(
        f_camPosWorldSpace,
        normalize((inverse(u_mvMat) * vec4(f_posCamSpace, 0.0)).xyz)
    );

    randState = gl_FragCoord.xy / u_windowSize;

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
