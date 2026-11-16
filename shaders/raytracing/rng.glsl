#ifndef RNG_GLSL
#define RNG_GLSL

uniform float frame_seed;

// formula from paper
float g(float x, float mu, float s1, float s2) {
	float t = (x - mu) / (x < mu ? s1 : s2);
	return exp(-0.5 * t * t);
}

// CIE 1931 (Wyman, Sloan, Shirley 2013).
vec3 cieXYZ(float l) {
	return vec3(
		1.056 * g(l, 599.8, 37.9, 31.0) + 0.362 * g(l, 442.0, 16.0, 26.7) - 0.065 * g(l, 501.1, 20.4, 26.2),
		0.821 * g(l, 568.8, 46.9, 40.5) + 0.286 * g(l, 530.9, 16.3, 31.1),
		1.217 * g(l, 437.0, 11.8, 36.0) + 0.681 * g(l, 459.0, 26.0, 13.8));
}

//380-780 nm
vec3 wavelengthToRGB(float l) {
	const mat3 XYZ2RGB = mat3( 3.2406, -0.9689,  0.0557,    
	                          -1.5372,  1.8758, -0.2040,
	                          -0.4986,  0.0415,  1.0570);
	vec3 rgb = max(XYZ2RGB * cieXYZ(l), 0.0);
	return rgb / vec3(0.4403, 0.2884, 0.2732);
}

uint rngState;
float randf() {
	rngState = rngState * 1664525u + 1013904223u;
	return float(rngState >> 8) / float(1u << 24);
}
vec3 randomUnitVector() {
	float z = 1.0 - 2.0 * randf();
	float r = sqrt(max(0.0, 1.0 - z * z));
	float phi = 6.2831853 * randf();
	return vec3(r * cos(phi), r * sin(phi), z);
}
void updateRandState(int indx){
    rngState = indx * 1973u + uint(frame_seed) * 9277u + 1u;
}

#endif
