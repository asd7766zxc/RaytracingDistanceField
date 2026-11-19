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

uniform vec3 EYE_IOR;

vec3 eye_radiance;
vec3 eye_color;
bool eye_done;

void store_color(vec3 color) {
	eye_color = color;
	eye_done = true;
}

int cur = 0;
uniform float shadow_strength = 1.0;
float shadowRay(vec3 p) {
	vec3 L = lightPos - p;
	float distL = length(L);
	int id;
	float t = march(p + vec3(0.0, 0.02, 0.0), L / distL, id); 
	if (t < 0.0 || id == sphereId()) return 1.0;  
	return t < distL ? 0.0 : 1.0;
}

float get(int i,vec3 p){
	if(i == 0) return p.x;
	if(i == 1) return p.y;
	return p.z;
}
bool hitglass = false;
bool onHit(inout RayState s, Hit h) {
	if (h.material == MAT_SPECULAR) {
		reflectRay(s, h);
		eye_radiance *= h.brdf;
		return true;
	}
	if (h.material == MAT_DIELECTRIC) {
		vec3 rd = s.rd;
		hitglass = true;
		if (refractRay(s, h, get(cur,EYE_IOR))) {
			// s.rd = rd; //pass thru
			// s.ro = h.p + rd * 0.05;
		}
		
		return true;
	}
	if (h.material == MAT_GROUND) {
		float diffuse = max(0.5 * dot(h.n, normalize(lightPos - h.p)), 0.0) * mix(1.0,shadowRay(h.p),shadow_strength);
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
	vec3 colorfin;
	for(int i = 0; i < 3; ++i){//per color channel
		cur = i;
		eye_radiance = vec3(1);
		eye_done = false;
		RayState s = genRay(o, o - eye_coord, WAVELENGTH_MID);
		tracePath(s);
		if(i == 0) colorfin.x = (eye_done ? eye_color : vec3(0.0)).x;
		if(i == 1) colorfin.y = (eye_done ? eye_color : vec3(0.0)).y;
		if(i == 2) colorfin.z = (eye_done ? eye_color : vec3(0.0)).z;

		if(!hitglass && i == 0){
			colorfin = eye_done ? eye_color : vec3(0.0);
			break;
		}
	} 
	imageStore(imgOutput, px, vec4(colorfin, 1.0));
}

#endif
