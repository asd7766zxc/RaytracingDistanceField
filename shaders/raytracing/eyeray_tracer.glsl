#ifndef EYERAY_TRACER_GLSL
#define EYERAY_TRACER_GLSL

#define MAX_DEPTH 50

#include "raytracing/scene.glsl"
#include "raytracing/caustic_map.glsl"
#include "raytracing/raytracing_arbiter.glsl"

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
layout(rgba32f, binding = 0) uniform image2D imgOutput;

uniform int window_w, window_h;
uniform vec3 pixel00Coord, du, dv, eye_coord;

const float EYE_IOR = 1.5;

vec3 eye_radiance;
vec3 eye_color;
bool eye_done;

void store_color(vec3 color) {
	eye_color = color;
	eye_done = true;
}

bool onHit(inout RayState s, Hit h) {
	if (h.material == MAT_SPECULAR) {
		reflectRay(s, h);
		eye_radiance *= h.brdf;
		return true;
	}
	if (h.material == MAT_DIELECTRIC) {
		vec3 rd = s.rd;
		if (refractRay(s, h, EYE_IOR)) {
			// s.rd = rd; //pass thru
			// s.ro = h.p + rd * 0.05;
		}
		
		return true;
	}
	if (h.material == MAT_GROUND) {
		float diffuse = max(0.5 * dot(h.n, normalize(lightPos - h.p)), 0.0);
		// diffuse = 0.0f;
		store_color(eye_radiance * diffuse + readCausticMap(h.p));
		return false;
	}
	store_color(eye_radiance);
	return false;
}

void onMiss(inout RayState s) {
	store_color(vec3(0)); //ambient
}

void main() {
	ivec2 px = ivec2(gl_GlobalInvocationID.xy);
	if (px.x >= window_w || px.y >= window_h) return;
	vec3 o = float(px.x) * du + float(px.y) * dv + pixel00Coord;

	eye_radiance = vec3(1);
	eye_done = false;
	RayState s = genRay(o, o - eye_coord, WAVELENGTH_MID);
	tracePath(s);
	imageStore(imgOutput, px, vec4(eye_done ? eye_color : eye_radiance, 1.0));
}

#endif
