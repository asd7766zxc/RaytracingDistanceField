#version 430 core

layout(binding = 0) uniform sampler3D distanceField;
in vec2 TexCoords;
out vec4 color;
uniform float slice;
uniform float max_dist;

void main() {
    float v = texture(distanceField,vec3((TexCoords.x) * 0.5f,slice,(1.0 - TexCoords.y) * 0.5f)).r;
    if(v >= 0){
        color = vec4(vec3(pow(1.0 - v,10)), 1.0f);
    }else{
         color = mix(vec4(pow(1.0 + v,10),1.0f,0.0f,1.0f),vec4(vec3(pow(1.0 + v,10)),1.0f),-v);
    }
}