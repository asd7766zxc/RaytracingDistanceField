#version 430 core

in vec3 pixelPos;
in vec3 pixelNorm;
in vec2 TexCoord;
uniform vec3 view_position;
uniform vec4 solid_color;
out vec4 color;

void main(){
	vec3 lightPos = vec3(0.0, 10.0, 0.0);
	vec3 L = normalize(lightPos - pixelPos);
	vec3 N = normalize(pixelNorm);
	vec3 V = normalize(view_position - pixelPos);
	vec3 diffuse = max(dot(N, L), 0.3) * vec3(1.0, 1.0, 1.0);
	vec3 R = reflect(-L, N);
	vec3 specular = pow(max(dot(R, V), 0.0),10) * vec3(1.0, 1.0, 1.0);
	color = solid_color * vec4(diffuse + specular, 1.0);
}