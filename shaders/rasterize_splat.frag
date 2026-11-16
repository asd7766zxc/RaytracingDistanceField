#version 430 core
layout (r32ui, binding = 6) uniform uimage2DArray caustic_map; // layer = R/G/B

flat in float density;
uniform float energy_scale;
uniform int channel; // 0/1/2 = R/G/B layer

void main(){
	uint v = uint(clamp(density * energy_scale, 0.0, 1e8));
	if(v > 0u) imageAtomicAdd(caustic_map, ivec3(gl_FragCoord.xy, channel), v);
}
