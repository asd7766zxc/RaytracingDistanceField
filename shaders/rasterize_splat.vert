#version 430 core

struct vert{
	vec4 pos;   
	vec4 path;  
};

layout (std430, binding = 7) readonly buffer splatted_list {
	vert splatted[];
};
const ivec2 corner[6] = ivec2[](ivec2(0,0), ivec2(1,0), ivec2(1,1),
                                 ivec2(0,0), ivec2(1,1), ivec2(0,1));
uniform int grid_size;

// ground is y = 0
// ground as texture
uniform vec3 groundMin; //min corner
uniform vec3 groundMax;

uniform float intensity;     
uniform vec2 map_size;       
uniform float min_area_px;   
uniform float max_area_px;   

flat out float density;

const float PI = 3.14159265;

int getID(ivec2 pos){
	return pos.x * grid_size + pos.y;
}
vec2 groundUV(vec3 p){
	return (p.xz - groundMin.xz) / (groundMax - groundMin).xz;
}
// Van Oosterom & Strackee: solid angle of the spherical triangle (a,b,c)
float solidAngle(vec3 a, vec3 b, vec3 c){
	float num = abs(dot(a, cross(b, c)));
	float den = 1.0 + dot(a, b) + dot(b, c) + dot(c, a);
	return 2.0 * atan(num, den);
}

void main(){
	int cells = grid_size - 1;
	int cur_cell = gl_VertexID / 6;
	int now_cor = gl_VertexID % 6;
	int tri_base = (now_cor / 3) * 3;
	ivec2 grid_pos = ivec2(cur_cell / cells, cur_cell % cells); // i,j

	vert v0 = splatted[getID(grid_pos + corner[tri_base + 0])];
	vert v1 = splatted[getID(grid_pos + corner[tri_base + 1])];
	vert v2 = splatted[getID(grid_pos + corner[tri_base + 2])];
	vert me = splatted[getID(grid_pos + corner[now_cor])];

	gl_Position = vec4(groundUV(me.pos.xyz) * 2.0 - 1.0, 0.0, 1.0);
	density = 0.0;

	bool valid = v0.pos.w > 0.0 && v1.pos.w > 0.0 && v2.pos.w > 0.0       
	          && v0.path.w == v1.path.w && v0.path.w == v2.path.w;   // took same path     

	vec2 a = groundUV(v0.pos.xyz) * map_size;
	vec2 b = groundUV(v1.pos.xyz) * map_size;
	vec2 c = groundUV(v2.pos.xyz) * map_size;
	vec2 e1 = b - a, e2 = c - a;
	float area = 0.5 * abs(e1.x * e2.y - e1.y * e2.x);
	valid = valid && area <= max_area_px;

	if(!valid){
		gl_Position = vec4(2.0, 2.0, 2.0, 1.0); // culled
		return;
	}

	float omega = solidAngle(normalize(v0.path.xyz), normalize(v1.path.xyz), normalize(v2.path.xyz));
	float w = (v0.pos.w + v1.pos.w + v2.pos.w) / 3.0;
	float energy = intensity * abs(omega) * w;
	density = energy / max(area, min_area_px);
}
