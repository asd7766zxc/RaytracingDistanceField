#version 430 core

layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vTexCoords;

uniform mat4 model;
out vec3 transformed_position;

void main(){
	transformed_position = (model * vec4(vPos, 1.0)).xyz;
}