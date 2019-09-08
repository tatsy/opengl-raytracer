#version 410
#extension GL_ARB_gpu_shader_fp64 : enable

#define ENABLE_VOLUME 0

//#define USE_DOUBLE
#ifdef USE_DOUBLE
#define Float double
#define Vec2 dvec2
#define Vec3 dvec3
#define Vec4 dvec4
const Float INFTY = 1.0e8;
const Float EPS = 1.0e-4;
#else
#define Float float
#define Vec2 vec2
#define Vec3 vec3
#define Vec4 vec4
const Float INFTY = 1.0e8;
const Float EPS = 1.0e-4;
#endif

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec4 out_count;

// ----------------------------------------------------------------------------
// Material types
// ----------------------------------------------------------------------------
const int MTRL_EMITTER = 0x01;
const int MTRL_DIFFUSE = 0x02;
const int MTRL_CONDUCTOR = 0x03;
const int MTRL_DIELECTRIC = 0x04;
const int MTRL_MEDIA = 0x05;

// ----------------------------------------------------------------------------
// Uniform variables
// ----------------------------------------------------------------------------

// Camera parameters
uniform mat4 u_c2wMat;
uniform mat4 u_s2cMat;
uniform float u_apertureRadius = 0.0;
uniform float u_focalLength = 1.0;

// Rendering parameters
uniform vec2 u_seed;
uniform int u_maxDepth = 16;
uniform int u_nSamples = 16;

// Frame
uniform vec2 u_windowSize;
uniform sampler2D u_framebuffer;
uniform sampler2D u_counter;

// Scene
uniform int u_nTris;
uniform samplerBuffer u_vertBuffer;
uniform samplerBuffer u_triBuffer;
uniform samplerBuffer u_matBuffer;
uniform samplerBuffer u_bvhBuffer;

// Light source
uniform int u_nLights;
uniform samplerBuffer u_lightBuffer;

// Volume
uniform bool u_hasVolume = false;
uniform float u_densityMax = 1.0;
uniform vec3 u_bboxMin = vec3(0.0);
uniform vec3 u_bboxMax = vec3(1.0);
uniform sampler3D u_densityTex;
uniform sampler3D u_temperatureTex;

// Constant parameters
const Float PI = 3.1415926535897932384626433832795;

// ----------------------------------------------------------------------------
// Structs
// ----------------------------------------------------------------------------

struct Ray {
    Vec3 o;
    Vec3 d;
};

struct Triangle {
    Vec3 v[3];
    Vec3 n[3];
};

struct Intersection {
    Vec3 wo;
    Vec3 norm;
    Float tHit;
    int mtrl;
};

// ----------------------------------------------------------------------------
// Random number generator
// ----------------------------------------------------------------------------

Vec2 randState;

Float rand() {
    Float a = 12.9898;
    Float b = 78.233;
    Float c = 43758.5453;
    randState.x = fract(sin(float(dot(randState.xy - u_seed, Vec2(a, b)))) * c);
    randState.y = fract(sin(float(dot(randState.xy - u_seed, Vec2(a, b)))) * c);
    return randState.x;
}

// ----------------------------------------------------------------------------
// Utility methods
// ----------------------------------------------------------------------------

bool isBlack(in Vec3 x) {
    return length(x) == 0.0;
}

Ray spawnRay(in Vec3 org, in Vec3 n, in Vec3 dir) {
    return Ray(org + n * EPS, dir);
}

Float blackBody(Float l, Float T) {
    Float h = 6.6260e-34;
    Float c = 2.9979e8;
    Float k = 1.3806e-23;
    Float l5 = (l * l) * (l * l) * l;
    return (2.0 * h * c * c) / (l5 * (exp(float((h * c) / (l * k  * T))) - 1.0));
}

Vec3 blackBody(Float T) {
    Float lambdaR = 6.10e-7;
    Float lambdaG = 5.50e-7;
    Float lambdaB = 4.50e-7;

    Float R = max(0.0, blackBody(lambdaR, T));
    Float G = max(0.0, blackBody(lambdaG, T));
    Float B = max(0.0, blackBody(lambdaB, T));
    return Vec3(R, G, B);
}

Float densityLookup(in Vec3 pos) {
    Vec3 uvw = (pos - u_bboxMin) / (u_bboxMax - u_bboxMin);
    return texture(u_densityTex, vec3(uvw)).x;
}

Float temperatureLookup(in Vec3 pos) {
    Vec3 uvw = (pos - u_bboxMin) / (u_bboxMax - u_bboxMin);
    return texture(u_temperatureTex, vec3(uvw)).x;
}

// ----------------------------------------------------------------------------
// BSDFs
// ----------------------------------------------------------------------------

Vec3 fresnelConductor(Float cosThetaI, Vec3 eta, Vec3 k) {
    Float cosThetaI2 = cosThetaI * cosThetaI;
    Float sinThetaI2 = 1.0 - cosThetaI2;
    Float sinThetaI4 = sinThetaI2*sinThetaI2;
    Vec3 eta2 = eta * eta;
    Vec3 k2 = k * k;

    Vec3 temp0 = eta2 - k2 - sinThetaI2;
    Vec3 a2pb2 = sqrt(max(Vec3(0.0), temp0 * temp0 + 4.0 * k2 * eta2));
    Vec3 a = sqrt(max(Vec3(0.0), (a2pb2 + temp0) * 0.5));

    Vec3 temp1 = a2pb2 + Vec3(cosThetaI2);
    Vec3 temp2 = 2.0 * a * cosThetaI;
    Vec3 Rs2 = (temp1 - temp2) / (temp1 + temp2);

    Vec3 temp3 = a2pb2 * cosThetaI2 + Vec3(sinThetaI4);
    Vec3 temp4 = temp2 * sinThetaI2;
    Vec3 Rp2 = Rs2 * (temp3 - temp4) / (temp3 + temp4);

    return 0.5 * (Rp2 + Rs2);
}

Float GGX(in Vec3 wh, in Vec2 alpha) {
    Vec3 whShrink = Vec3(wh.x / alpha.x, wh.y / alpha.y, wh.z);
    Float l2 = dot(whShrink, whShrink);
    return 1.0 / (PI * (alpha.x * alpha.y) * l2 * l2);
}

Float microfacetGGXBRDF(in Vec3 wi, in Vec3 wo, in Vec2 alpha) {
    Vec3 wh = normalize(wi + wo);
    Float zi = abs(wi.z);
    Float zo = abs(wo.z);
    Vec3 wiStretch = Vec3(wi.x * alpha.x, wi.y * alpha.y, wi.z);
    Vec3 woStretch = Vec3(wo.x * alpha.x, wo.y * alpha.y, wo.z);
    return GGX(wh, alpha) / (2.0 * (zo * length(wiStretch) + zi * length(woStretch)));
}

Vec3 sampleGGXVNDF(in Vec3 ve, in Vec2 alpha, in Vec2 u) {
    // See "Sampling the GGX Distribution of Visible Normals" by E.Heitz in JCGT, 2018.
    Vec3 vh = normalize(vec3(ve.x * alpha.x, ve.y * alpha.y, ve.z));

    Float lensq = vh.x * vh.x + vh.y * vh.y;
    Vec3 T1 = lensq > 0.0 ? Vec3(-vh.y, vh.x, 0.0) * inversesqrt(lensq) : Vec3(1.0, 0.0, 0.0);
    Vec3 T2 = cross(vh, T1);

    Float r = sqrt(u.x);
    Float phi = 2.0 * PI * u.y;

    Float t1 = r * cos(float(phi));
    Float t2 = r * sin(float(phi));
    Float s = 0.5 * (1.0 + vh.z);
    t2 = (1.0 - s) * sqrt(1.0 - t1 * t1) + s * t2;

    Vec3 nh = t1 * T1 + t2 * T2 + sqrt(max(0.0, 1.0 - t1 * t1 - t2 * t2)) * vh;
    Vec3 ne = normalize(Vec3(nh.x * alpha.x, nh.y * alpha.y, max(0.0, nh.z)));
    return ne;
}

Float weightedGGXPDF(in Vec3 wi, in Vec3 wo, in Vec3 wh, in Vec2 alpha) {
    Vec3 woStretch = vec3(wo.x * alpha.x, wo.y * alpha.y, wo.z);
    return 0.5 / (length(woStretch) + wo.z) * GGX(wh, alpha) * max(0.0, dot(wo, wh)) / max(EPS, dot(wi, wh));
}


// ----------------------------------------------------------------------------
// Intersection test
// ----------------------------------------------------------------------------

Float intersect(in Ray ray, in Triangle tri, out Vec3 norm) {
    Vec3 e1 = tri.v[1] - tri.v[0];
    Vec3 e2 = tri.v[2] - tri.v[0];
    Vec3 pVec = cross(ray.d, e2);

    Float det = dot(e1, pVec);
    if (det > -EPS && det < EPS) {
        return INFTY;
    }

    Float invdet = 1.0 / det;
    Vec3 tVec = ray.o - tri.v[0];
    Float u = dot(tVec, pVec) * invdet;
    if (u < 0.0 || u > 1.0) {
        return INFTY;
    }

    Vec3 qVec = cross(tVec, e1);
    Float v = dot(ray.d, qVec) * invdet;
    if (v < 0.0 || u + v > 1.0) {
        return INFTY;
    }

    Float t = dot(e2, qVec) * invdet;
    if (t <= EPS) {
        return INFTY;
    }

    norm = normalize((1.0 - u - v) * tri.n[0] + u * tri.n[1] + v * tri.n[2]);

    return t;
}

bool intersectBBox(in Ray ray, Vec3 posMin, Vec3 posMax, out Float tMin, out Float tMax) {
    Vec3 invdir = Vec3(1.0) / ray.d;

    Vec3 f = (posMax - ray.o) * invdir;
    Vec3 n = (posMin - ray.o) * invdir;

    Vec3 tmax = max(f, n);
    Vec3 tmin = min(f, n);

    Float t1 = min(tmax.x, min(tmax.y, tmax.z));
    Float t0 = max(tmin.x, max(tmin.y, tmin.z));

    tMin = t0;
    tMax = t1;
    return t1 >= t0;
}

bool intersect(in Ray ray, out Intersection isect) {
    isect.tHit = INFTY;
    isect.wo = -ray.d;
    isect.norm = Vec3(0.0);
    isect.mtrl = 0;

    bool hit = false;
    int pos = 0;
    int stack[64];

    stack[0] = 0;
    while (pos >= 0) {
        int node = stack[pos];
        pos -= 1;

        Vec3 posMin = texelFetch(u_bvhBuffer, node * 3 + 0).xyz;
        Vec3 posMax = texelFetch(u_bvhBuffer, node * 3 + 1).xyz;
        Vec3 children = texelFetch(u_bvhBuffer, node * 3 + 2).xyz;
        if (children.z < 0.0) {
            // Fork node
            Float tMin, tMax;
            if (intersectBBox(ray, posMin, posMax, tMin, tMax)) {
                if (tMin <= isect.tHit) {
                    if (children.x >= 0.0) {
                        stack[pos + 1] = int(children.x);
                        pos += 1;
                    }

                    if (children.y >= 0.0) {
                        stack[pos + 1] = int(children.y);
                        pos += 1;
                    }
                }
            }
        } else {
            // Leaf node
            int index = int(children.z);
            Vec3 ijk = texelFetch(u_triBuffer, index).xyz;

            Triangle tri;
            tri.v[0] = texelFetch(u_vertBuffer, int(ijk.x) * 5 + 0).xyz;
            tri.v[1] = texelFetch(u_vertBuffer, int(ijk.y) * 5 + 0).xyz;
            tri.v[2] = texelFetch(u_vertBuffer, int(ijk.z) * 5 + 0).xyz;
            tri.n[0] = texelFetch(u_vertBuffer, int(ijk.x) * 5 + 1).xyz;
            tri.n[1] = texelFetch(u_vertBuffer, int(ijk.y) * 5 + 1).xyz;
            tri.n[2] = texelFetch(u_vertBuffer, int(ijk.z) * 5 + 1).xyz;

            Vec3 n;
            Float dist = intersect(ray, tri, n);
            if (dist < isect.tHit) {
                isect.tHit = dist;
                isect.norm = n;
                isect.mtrl = int(texelFetch(u_triBuffer, index).w);
                hit = true;
            }
        }
    }

    return hit;
}

Vec3 sampleDirect(in Vec3 x, in Intersection isect) {
    // Take sample vertex on an area light
    int lightID = min(int(rand() * u_nLights), u_nLights - 1);
    Vec3 ijk = texelFetch(u_lightBuffer, lightID).xyz;

    Triangle tri;
    tri.v[0] = texelFetch(u_vertBuffer, int(ijk.x) * 5 + 0).xyz;
    tri.v[1] = texelFetch(u_vertBuffer, int(ijk.y) * 5 + 0).xyz;
    tri.v[2] = texelFetch(u_vertBuffer, int(ijk.z) * 5 + 0).xyz;
    tri.n[0] = texelFetch(u_vertBuffer, int(ijk.x) * 5 + 1).xyz;
    tri.n[1] = texelFetch(u_vertBuffer, int(ijk.y) * 5 + 1).xyz;
    tri.n[2] = texelFetch(u_vertBuffer, int(ijk.z) * 5 + 1).xyz;

    Vec2 u = Vec2(rand(), rand());
    if (u.x + u.y > 1.0) {
        u.x = 1.0 - u.x;
        u.y = 1.0 - u.y;
    }
    Vec3 p = (1.0 - u.x - u.y) * tri.v[0] + u.x * tri.v[1] + u.y * tri.v[2];
    Vec3 nl = (1.0 - u.x - u.y) * tri.n[0] + u.x * tri.n[1] + u.y * tri.n[2];

    // Cast shadow ray
    Vec3 dir = normalize(p - x);
    Ray ray = spawnRay(x, isect.norm, dir);

    Intersection temp;
    bool isHit = intersect(ray, temp);

    // Calculate contribution
    Float dist = length(p - x);
    if (isHit && abs(dist - temp.tHit) < EPS) {
        // Evaluate BRDF
        int type = int(texelFetch(u_matBuffer, isect.mtrl * 6 + 0).x);
        Vec3 f = Vec3(0.0);
        if (type == MTRL_DIFFUSE) {
            f = texelFetch(u_matBuffer, isect.mtrl * 6 + 2).xyz;
        } else if (type == MTRL_CONDUCTOR) {
            Vec3 kappa = texelFetch(u_matBuffer, isect.mtrl * 6 + 2).xyz;
            Vec3 eta = texelFetch(u_matBuffer, isect.mtrl * 6 + 3).xyz;
            Vec2 alpha = texelFetch(u_matBuffer, isect.mtrl * 6 + 4).xy;
            Float cosThetaI = max(0.0, dot(isect.norm, -ray.d));
            Vec3 F = fresnelConductor(cosThetaI, eta, kappa);

            Vec3 w = isect.norm;
            Vec3 u = cross(abs(w.x) > 0.1 ? Vec3(0.0, 1.0, 0.0) : Vec3(1.0, 0.0, 0.0), w);
            Vec3 v = cross(w, u);
            Vec3 wo = isect.wo;
            Vec3 wi = ray.d;
            Vec3 woLocal = Vec3(dot(u, wo), dot(v, wo), dot(w, wo));
            Vec3 wiLocal = Vec3(dot(u, wi), dot(v, wi), dot(w, wi));
            f = F * microfacetGGXBRDF(wiLocal, woLocal, alpha);
        }

        // Evaluate contribution
        int mtrlID = int(texelFetch(u_lightBuffer, lightID).w);
        Vec3 e = texelFetch(u_matBuffer, mtrlID * 6 + 1).xyz;
        Float dot0 = dot(ray.d, isect.norm);
        Float dot1 = dot(-ray.d, nl);
        if (dot0 > 0.0 && dot1 > 0.0) {
            Float G = (dot0 * dot1) / (dist * dist);
            Float area = 0.5 * length(cross(tri.v[1] - tri.v[0], tri.v[2] - tri.v[0]));
            Float pdf = 1.0 / (area * Float(u_nLights));
            return e * f * G / pdf;
        }
    }
    return Vec3(0.0);
}

// ----------------------------------------------------------------------------
// Radiance
// ----------------------------------------------------------------------------

Vec3 radiance(in Ray r){
    Vec3 L = Vec3(0.0);
    Vec3 beta = Vec3(1.0);
    Ray ray = r;
    bool passedVolume = false;
    bool specularReflect = false;

    for (int depth = 0; depth < u_maxDepth; depth++) {
        Intersection isect;
        bool isIntersect = intersect(ray, isect);

        Vec3 x = ray.o + (isect.tHit + EPS) * ray.d;
        int type = int(texelFetch(u_matBuffer, isect.mtrl * 6 + 0).x);
        Vec3 e = texelFetch(u_matBuffer, isect.mtrl * 6 + 1).xyz;

        if (type == MTRL_MEDIA && dot(-ray.d, isect.norm) >= EPS) {
            #if ENABLE_VOLUME
            // Volume (perform Woodcock tracking)
            Vec3 sigS = Vec3(1.0, 1.0, 1.0) * 0.1;
            Vec3 sigA = Vec3(0.1, 0.1, 0.1) * 0.6;
            Float sigT = sigS.x + sigA.x;

            int nTrial = 8;
            Vec3 nextOrg = x;
            Vec3 nextDir = ray.d;
            for (int k = 0; k < nTrial; k++) {
                Ray nextRay = spawnRay(nextOrg, vec3(0.0), nextDir);
                Intersection inext;
                bool hit = intersect(nextRay, inext);
                if (!hit) {
                    return Vec3(1, 0, 1);
                }

                Float t = 0.0;
                bool pass = false;
                for (int i = 0; i < 256; i++) {
                    t -= log(max(EPS, 1.0 - rand())) / (u_densityMax * sigT);
                    if (t >= inext.tHit) {
                        pass = true;
                        break;
                    }

                    Vec3 pos = nextOrg + t * nextDir;
                    Float density = densityLookup(pos);
                    if (density / u_densityMax > rand()) {
                        pass = true;
                        break;
                    }
                }

                if (!pass) {
                    return Vec3(0.0, 0.0, 0.0);
                }

                if (t >= inext.tHit) {
                    nextOrg = nextOrg + (inext.tHit + EPS) * nextDir;
                    break;
                }

                nextOrg = nextOrg + t * nextDir;

                // Black body radiation
                Float T = temperatureLookup(nextOrg) * 1.0e2;
                L += beta * blackBody(T) / sigT;

                // Sample path direction
                Float theta = acos(2.0 * rand() - 1.0);
                Float phi = 2.0 * PI * rand();
                Float cosTheta = cos(theta);
                Float sinTheta = sin(theta);
                nextDir = Vec3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);

                // Scattering albedo
                beta *= sigS / sigT;
            }

            ray = spawnRay(nextOrg, vec3(0.0), nextDir);
            passedVolume = true;
            #endif
        } else {
            // Surface
            if (depth == 0 || specularReflect || passedVolume) {
                if (isIntersect) {
                    L += beta * e;
                }
            }
            passedVolume = false;

            if (!isIntersect) {
                break;
            }

            // Sample BRDF
            Vec3 w = isect.norm;
            Vec3 u = cross(abs(w.x) > 0.1 ? Vec3(0.0, 1.0, 0.0) : Vec3(1.0, 0.0, 0.0), w);
            Vec3 v = cross(w, u);
            Vec3 wo = -ray.d;
            Vec3 woLocal = Vec3(dot(u, wo), dot(v, wo), dot(w, wo));

            Vec3 f = Vec3(0.0);
            Float pdf = 1.0f;
            Vec3 wiLocal = Vec3(0.0, 0.0, 1.0);
            if (type == MTRL_DIFFUSE) {
                Float r1 = 2.0 * PI * rand();
                Float r2 = rand();
                Float r2s = sqrt(r2);

                wiLocal = Vec3(cos(float(r1)) * r2s, sin(float(r1)) * r2s, sqrt(1.0 - r2));
                f = texelFetch(u_matBuffer, isect.mtrl * 6 + 2).xyz / PI;
                pdf = wiLocal.z / PI;
                specularReflect = false;
            } else if (type == MTRL_CONDUCTOR) {
                Vec3 kappa = texelFetch(u_matBuffer, isect.mtrl * 6 + 2).xyz;
                Vec3 eta = texelFetch(u_matBuffer, isect.mtrl * 6 + 3).xyz;
                Vec2 alpha = texelFetch(u_matBuffer, isect.mtrl * 6 + 4).xy;
                Vec2 u2 = Vec2(rand(), rand());
                Vec3 whLocal = sampleGGXVNDF(woLocal, alpha, u2);

                wiLocal = 2.0 * dot(whLocal, woLocal) * whLocal - woLocal;
                Vec3 F = fresnelConductor(wiLocal.z, eta, kappa);
                f = F * microfacetGGXBRDF(wiLocal, woLocal, alpha);
                pdf = weightedGGXPDF(wiLocal, woLocal, whLocal, alpha);
                specularReflect = false;
            }

            if (isBlack(f) || pdf == 0.0) {
                break;
            }

            // Direct lighting
            L += beta * sampleDirect(x, isect);

            // Update ray and beta
            Vec3 wi = u * wiLocal.x + v * wiLocal.y + w * wiLocal.z;
            ray = spawnRay(x, isect.norm, wi);
            beta *= f * max(0.0, dot(isect.norm, wi)) / pdf;

        }

        // Russian roulette
        if (depth > 2) {
            Float p = min(0.95, max(beta.x, max(beta.y, beta.z)));
            if (rand() > p) {
                break;
            }
            beta /= p;
        }
    }

    return min(L, 1.0e2);
}

// ----------------------------------------------------------------------------
// Main
// ----------------------------------------------------------------------------

void main(void) {
    // Initialize random number state
    randState = gl_FragCoord.xy / u_windowSize;

    // Framebuffer settings
    Vec2 uv = gl_FragCoord.xy / u_windowSize;
    Vec3 L = texture(u_framebuffer, vec2(uv)).rgb;
    Float count = texture(u_counter, vec2(uv)).x;

    // Main loop
    for (int i = 0; i < u_nSamples; i++) {
        // Camera space
        Vec2 pRand = Vec2(rand(), rand());
        Vec3 pScreen = Vec3(0.0);
        pScreen.xy = (gl_FragCoord.xy + pRand) / u_windowSize;
        pScreen.xy = pScreen.xy * 2.0 - 1.0;

        Vec4 temp;
        temp = u_s2cMat * Vec4(pScreen, 1.0);
        Vec3 pCamera = temp.xyz / temp.w;
        Vec3 org = Vec3(0.0, 0.0, 0.0);
        Vec3 dir = normalize(pCamera);

        // Sample on disk
        if (u_apertureRadius > 0.0) {
            Float r = sqrt(rand()) * u_apertureRadius;
            Float theta = rand() * 2.0 * PI;
            Vec2 pLens = Vec2(r * cos(float(theta)), r * sin(float(theta)));
            Float ft = -u_focalLength / dir.z;
            Vec3 pFocus = org + dir * ft;

            org = Vec3(pLens, 0.0);
            dir = normalize(pFocus - org);
        }

        // World space
        temp = u_c2wMat * Vec4(org, 1.0);
        Vec3 orgWorld = temp.xyz / temp.w;
        temp = u_c2wMat * Vec4(dir, 0.0);
        Vec3 dirWorld = temp.xyz;

        // Ray tracing
        Ray ray = Ray(orgWorld, normalize(dirWorld));
        L += radiance(ray);
        count += 1.0;
    }

    out_color = vec4(L, 1.0);
    out_count = vec4(count, count, count, 1.0);
}
