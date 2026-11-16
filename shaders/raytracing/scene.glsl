#ifndef SCENE_GLSL
#define SCENE_GLSL

#ifndef MARCH_STEPS
#define MARCH_STEPS 50
#endif

#define MAT_DIFFUSE    0
#define MAT_SPECULAR   1
#define MAT_EMISSIVE   2
#define MAT_DIELECTRIC 3
#define MAT_GROUND     4

struct RayState {
	vec3 ro, rd;
	float t;
	bool refracted;
	bool inside_media;
	float path_hash;
	float wave_length; //[0,1]
	float radiance;
	int depth;
};

struct Hit {
	vec3 p, n;
	bool inside;
	int id;
	int material;
	vec3 brdf;
};

struct df_object {
	mat4 worldToModel;
	uvec2 texture_handle;
	vec4 df_scale;
	vec4 material;
	vec4 brdf;
	ivec4 type;
	float df_size;
};

layout(std430, binding = 2) buffer DistanceFieldHandles { df_object objects[]; };

uniform vec3 groundMin, groundMax;
uniform int objects_count;
uniform vec3 lightPos, intensity;

const vec3 BOX_CENTER = vec3(3, 0, -5);
const vec3 BOX_HALF = vec3(4, 5, 1);
const vec3 SPHERE_brdf = vec3(0.7, 0.6, 0.5);

int sphereId() { return objects_count; }
int groundId() { return objects_count + 1; }

const float WAVELENGTH_MIN_NM = 380.0;
const float WAVELENGTH_MAX_NM = 780.0;
const float WAVELENGTH_MID = 0.425;

const float LIGHT_RADIUS = 0.5;

float boxSDF(vec3 p){
	vec3 b = abs(p - BOX_CENTER) - BOX_HALF;
	return length(max(b, 0.0)) + min(max(b.x, max(b.y, b.z)), 0.0);
}
float sphereSDF(vec3 p){
	return length(p - lightPos) - LIGHT_RADIUS;
}

vec3 lightSurface(vec3 dir) {
	return lightPos + normalize(dir) * (LIGHT_RADIUS + 0.02);
}

float wavelengthToNM(float wave_length) {
	return mix(WAVELENGTH_MIN_NM, WAVELENGTH_MAX_NM, wave_length);
}

RayState genRay(vec3 ro, vec3 rd, float wave_length) {
	return RayState(ro, normalize(rd), 0.0, false, false, 0.0, clamp(wave_length, 0.0, 1.0), 1.0, 0);
}

float sceneSDF(vec3 p, out int id) {
	float d = 1e4;
	id = -1;
	for (int i = 0; i < objects_count; ++i) {
		vec3 q = (objects[i].worldToModel * vec4(p, 1.0)).xyz;
		float di = texture(sampler3D(objects[i].texture_handle), q / objects[i].df_scale.xyz).r;
		if (di < d) { d = di; id = i; }
	}
	float sd = sphereSDF(p);
	if(sd < d){
		d = sd;
		id = sphereId();
	}
	return d;
}
float sceneSDF(vec3 p) { int id; return sceneSDF(p, id); }

vec3 sceneNormal(vec3 p) {
	const vec2 h = vec2(0.01, 0);
	return normalize(vec3(sceneSDF(p + h.xyy) - sceneSDF(p - h.xyy),
	                      sceneSDF(p + h.yxy) - sceneSDF(p - h.yxy),
	                      sceneSDF(p + h.yyx) - sceneSDF(p - h.yyx)));
}

float march(vec3 ro, vec3 rd, out int id) {
	float tg = rd.y < 0.0 ? -ro.y / rd.y : 1e9;
	float t = 0.0, d = 1e4;
	id = -1;
	for (int s = 0; s < MARCH_STEPS && d > 1e-2 && t < tg; ++s) {
		d = abs(sceneSDF(ro + rd * t, id));
		t += d;
	}
	if (d <= 1e-2 && t < tg) return t;
	if (t >= tg) { id = groundId(); return tg; }
	return -1.0;
}


bool intersect(inout RayState s, out Hit h) {
	int id;
	float t = march(s.ro, s.rd, id);
	h.id = id;
	if (t < 0.0) return false;
	s.t = t;
	h.p = s.ro + s.rd * t;
	h.n = id == groundId() ? vec3(0, 1, 0) : sceneNormal(h.p);
	h.inside = dot(h.n, s.rd) >= 0.0;
	if (h.inside) h.n = -h.n;
	if (id == groundId()) { h.material = MAT_GROUND; h.brdf = vec3(1); }
	else if (id == sphereId()) { h.material = MAT_EMISSIVE; h.brdf = SPHERE_brdf; }
	else { h.material = objects[id].type.x; h.brdf = vec3(1); }
	return true;
}

bool groundUV(vec3 p, out vec2 uv) {
	uv = (p.xz - groundMin.xy) / (groundMax - groundMin).xy;
	return all(greaterThan(uv, vec2(0))) && all(lessThan(uv, vec2(1)));
}

#endif
