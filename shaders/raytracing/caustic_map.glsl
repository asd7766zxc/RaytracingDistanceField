#ifndef CAUSTIC_MAP_GLSL
#define CAUSTIC_MAP_GLSL

#include "raytracing/rng.glsl"
#include "raytracing/scene.glsl"

#ifdef CAUSTIC_LAYERED

layout(r32ui, binding = 6) uniform uimage2DArray caustic_layers;

ivec2 causticSize() { return imageSize(caustic_layers).xy; }
float causticTexel(ivec2 p, int c) { return float(imageLoad(caustic_layers, ivec3(p, c)).r); }

#else

layout(r32ui, binding = 3) uniform uimage2D caustic_map[3];

ivec2 causticSize() { return imageSize(caustic_map[0]); }
float causticTexel(ivec2 p, int c) {
	if (c == 0) return float(imageLoad(caustic_map[0], p).r);
	if (c == 1) return float(imageLoad(caustic_map[1], p).r);
	return float(imageLoad(caustic_map[2], p).r);
}

void causticAdd(int c, ivec2 p, float v) {
	if (c == 0) imageAtomicAdd(caustic_map[0], p, uint(v));
	else if (c == 1) imageAtomicAdd(caustic_map[1], p, uint(v));
	else imageAtomicAdd(caustic_map[2], p, uint(v));
}

//splat bilinearly
void splat(float wave_length, vec2 uv, float val) {
	vec2 fpos = uv * vec2(causticSize()) - 0.5;
	ivec2 p = ivec2(floor(fpos));
	vec2 f = fract(fpos), g = 1.0 - f;
	vec3 color = wavelengthToRGB(wavelengthToNM(wave_length));
	for (int c = 0; c < 3; ++c) {
		float v = color[c] * val;
		causticAdd(c, p, v * g.x * g.y);
		causticAdd(c, p + ivec2(1, 0), v * f.x * g.y);
		causticAdd(c, p + ivec2(0, 1), v * g.x * f.y);
		causticAdd(c, p + ivec2(1, 1), v * f.x * f.y);
	}
}

#endif

vec3 causticTexels(ivec2 p) {
	return vec3(causticTexel(p, 0), causticTexel(p, 1), causticTexel(p, 2));
}

vec3 readCausticMap(vec3 gpos) {
	vec2 uv;
	if (!groundUV(gpos, uv)) return vec3(0);
	vec2 fpos = uv * vec2(causticSize()) - 0.5;
	ivec2 p = ivec2(floor(fpos));
	vec2 f = fract(fpos);
	vec3 c = mix(mix(causticTexels(p), causticTexels(p + ivec2(1, 0)), f.x),
	             mix(causticTexels(p + ivec2(0, 1)), causticTexels(p + ivec2(1, 1)), f.x), f.y);
	vec2 s = vec2(causticSize()) / 2048.0;
	return c / 100000.0 * (s.x * s.y);
}

#endif
