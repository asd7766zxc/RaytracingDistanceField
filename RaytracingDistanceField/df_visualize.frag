#version 430 core

layout(binding = 0) uniform sampler3D distanceField;
in vec2 TexCoords;
out vec4 color;
uniform float slice;
uniform float max_dist;

void main() {
    float v = texture(distanceField,vec3(TexCoords.xy,slice)).r;
    v /= max_dist;
   if(v >= 0){
        color = vec4(vec3(pow(1.0 - v,10)), 1.0f);
    }else{
         color = mix(vec4(pow(1.0 + v,10),1.0f,0.0f,1.0f),vec4(vec3(pow(1.0 + v,10)),1.0f),-v);
    }

    color = vec4(vec3(v),1.0f);
}