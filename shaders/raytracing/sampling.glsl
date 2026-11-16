#ifndef SAMPLING_GLSL
#define SAMPLING_GLSL

#include "raytracing/rng.glsl"
#include "raytracing/scene.glsl"

const float IOR[3] = float[](1.3, 1.5, 1.4);
const vec3 DEFAULT_AIM = vec3(-1, 1.5, 1.6);

float sampleWavelength() {
	return randf();
}

float wavelengthIOR(float wave_length) {
	return mix(IOR[2], IOR[0], wave_length);
}

vec3 aimPoint() {
	return objects_count > 0
		? (inverse(objects[0].worldToModel) * vec4(0.5 * objects[0].df_scale.xyz, 1.0)).xyz
		: DEFAULT_AIM;
}

vec3 sampleTile(vec3 from, vec3 aim, vec2 tile) {
	vec3 w = normalize(aim - from);
	vec3 u = normalize(cross(vec3(0, 0, 1), w));
	vec3 v = cross(w, u);
	return normalize(w + tile.x * (2.0 * randf() - 1.0) * u + tile.y * (2.0 * randf() - 1.0) * v);
}

vec3 octDecode(vec2 uv) {
	vec2 p = uv * 2.0 - 1.0;
	vec3 n = vec3(p, 1.0 - abs(p.x) - abs(p.y));
	if (n.z < 0.0) n.xy = (1.0 - abs(n.yx)) * vec2(n.x >= 0.0 ? 1.0 : -1.0, n.y >= 0.0 ? 1.0 : -1.0);
	return normalize(n);
}

#endif
