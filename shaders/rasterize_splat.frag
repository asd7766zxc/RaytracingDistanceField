#version 430 core
layout (r32ui, binding = 6) uniform uimage2DArray caustic_map; // layer = R/G/B

flat in float density;
uniform float energy_scale;
uniform int channel; // wavelength index
uniform vec3 channel_weight;
void main(){
	vec3 e = density * energy_scale * channel_weight;
	for (int c = 0; c < 3; ++c) {
		uint v = uint(clamp(e[c] + 0.5, 0.0, 1e8));
		if (v > 0u) imageAtomicAdd(caustic_map, ivec3(gl_FragCoord.xy, c), v);
	}

}
