#ifndef RAYTRACING_ARBITER_GLSL
#define RAYTRACING_ARBITER_GLSL

#include "raytracing/scene.glsl"

#ifndef MAX_DEPTH
#define MAX_DEPTH 20
#endif

bool onHit(inout RayState s, Hit h);
void onMiss(inout RayState s);

void tracePath(inout RayState s) {
	for (s.depth = 0; s.depth < MAX_DEPTH; ++s.depth) {
		Hit h;
		if (!intersect(s, h)) { onMiss(s); return; }
		if (!onHit(s, h)) return;
	}
}

void reflectRay(inout RayState s, Hit h) {
	s.rd = reflect(s.rd, h.n);
	s.ro = h.p + h.n * 0.02;
}

bool refractRay(inout RayState s, Hit h, float ior) {
	float eta = h.inside ? ior : 1.0 / ior; // 1.0 for air
	float cs = min(dot(-s.rd, h.n), 1.0);
	bool tir = eta * sqrt(1.0 - cs * cs) > 1.0;
	s.rd = tir ? reflect(s.rd, h.n) : refract(s.rd, h.n, eta);
	s.ro = h.p + (tir ? h.n : -h.n) * 0.03 + s.rd * 0.01;
	s.refracted = true;
	if (!tir) s.inside_media = !s.inside_media;
	return tir;
}

#endif
