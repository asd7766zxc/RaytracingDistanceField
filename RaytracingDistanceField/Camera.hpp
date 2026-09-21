#pragma once
#include "Vec.hpp"

class Camera {
public:
	float fov = 90.0f;
	float nearp = .01f;
	float farp = 100.f;
	float aspect_ratio = 1.0f;
	vec3 du, dv;

	vec3 position = vec3(0, 0, 0);

	int width_resolution;
	int height_resolution;

	mat4 view, proj, cproj;

	vec3 vup = vec3(0, 1, 0);
	void lookAt(vec3 focus);
	void updateProj(int w, int h, float nearp, float farp, float fov);
	void make_ortho(int w, int h, float sz);
	void windowResize(int w, int h);
	vec3 getWorldMousePos(float mx, float my, int plane);
	mat4 getMatrix() const;
	vec3 getLookAt();
	//https://terathon.com/blog/oblique-clipping.html
	void ObliqueProj(vec3 pos, vec3 norm, bool clipOppo = false);

	vec3 u() const;
	vec3 v() const;
	vec3 w() const;

	vec3 getPixelCornerPos() const;
	vec3 getPixel00Pos();
};