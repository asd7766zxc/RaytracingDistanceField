#version 430 core

in vec3 pixelPos;
in vec3 pixelNorm;
in vec2 TexCoord;
uniform vec3 view_position;
uniform vec4 solid_color;
out vec4 color;

void main(){
	vec3 N = normalize(pixelNorm);
	vec3 diffuse = max(dot(N, vec3(1,1,1)), 0.2) * vec3(1.0, 1.0, 1.0);
	color = solid_color * vec4(diffuse, 1.0);
}