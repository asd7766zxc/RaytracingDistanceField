#include "Camera.hpp"

void Camera::lookAt(vec3 focus) {
	vec3 w = -(focus - position);
	vec3 u = vup ^ w;
	vec3 v = w ^ u;
	w = uni(w);
	u = uni(u);
	v = uni(v);
	view = mat4::coord(u, v, w) * mat4::trans(-position);
}
void Camera::updateProj(int w, int h, float nearp, float farp, float fov) {
	const float e = 1.0f / std::tan(fov * pi / 360.f);
	const float a = float(h) / float(w);
	const float d = nearp - farp;

	proj.makeZero();

	proj[0] = e * a;
	proj[5] = e;
	proj[10] = (farp + nearp) / d;
	proj[11] = (2 * farp * nearp) / d;
	proj[14] = -1.0f;
}

void Camera::make_ortho(int w, int h, float sz = 10) {
	float aspect_ratio = float(h) / float(w);
	float nearp = -0.0f;
	float farp = 100.0;
	float left = -sz;
	float right = sz;
	float bottom = -sz * aspect_ratio;
	float top = sz * aspect_ratio;
	proj.makeZero();
	proj[3] = -(right + left) / (right - left);
	proj[7] = -(top + bottom) / (top - bottom);
	proj[11] = -(farp + nearp) / (farp - nearp);

	proj[0] = 2 / (right - left);
	proj[5] = 2 / (top - bottom);
	proj[10] = -2 / (farp - nearp);
	proj[15] = 1;
	//std::cout << proj << '\n';
	return;
}

void Camera::windowResize(int w, int h) {
	width_resolution = w, height_resolution = h;
	aspect_ratio = (float)(w) / (float)(h);
	updateProj(w, h, nearp, farp, fov);
}
mat4 Camera::getMatrix() const {
	return proj * view;
}

vec3 Camera::getLookAt() {
	return (view * vec4(0, 0, -1, 0)).toVec3();
}
vec3 Camera::getWorldMousePos(float mx, float my, int plane) {
	return ((getMatrix()).inverse() * vec4(mx, my, plane, 1)).toVec3DivW();
}
//https://terathon.com/blog/oblique-clipping.html
void Camera::ObliqueProj(vec3 pos, vec3 norm, bool clipOppo) {
	cproj = proj;
	//far plane 在oblique clip space 會被忽略 但太大會造成數值誤差
	//所以直接開 1.0f 
	cproj[10] = (1.0f + 0.01f) / (0.01f - 1.0f);
	cproj[11] = (2 * 1.0f * 0.01f) / (0.01f - 1.0f);

	vec3 cpos = (view * vec4(pos, 1)).toVec3();
	vec3 cnorm = (view * vec4(norm, 0)).toVec3();
	vec4 plane = vec4(cnorm.x, cnorm.y, cnorm.z, -(cnorm * cpos));
	vec4 q = cproj.inverse() * vec4(
		plane.x < 0.0f ? 1.0f : -1.0f,
		plane.y < 0.0f ? 1.0f : -1.0f,
		1.0f, 1.0f);
	vec4 c = plane * (2.0f / -(plane * q));

	if (clipOppo) c = c * (-1.0f);

	// 注意 replace 後 inverse 會變。
	// 所以直接開一個新的
	cproj.mt[8] = c.x;
	cproj.mt[9] = c.y;
	cproj.mt[10] = c.z + 1.0f;
	cproj.mt[11] = c.w;
}

vec3 Camera::u() const {
	return vec3(view.mt[0], view.mt[1], view.mt[2]);
}

vec3 Camera::v() const {
	return vec3(view.mt[4], view.mt[5], view.mt[6]);
}

vec3 Camera::w() const {
	return -vec3(view.mt[8], view.mt[9], view.mt[10]);
}
vec3 Camera::getPixelCornerPos() const {
	vec3 nz = w();
	nz *= nearp;
	float width = tan(fov / 2) * nearp;
	float height = width / aspect_ratio;

	return (-width * u()) + (height * v()) + nz + position;
}
vec3 Camera::getPixel00Pos() {
	vec3 cpos = getPixelCornerPos();

	float width = 2 * tan(fov / 2) * nearp;
	float height = width / aspect_ratio;

	float dwidth = width / width_resolution;
	float dheight = height / height_resolution;

	du = dwidth * u();
	dv = dheight * (-v());
	return cpos + 0.5 * du + 0.5 * dv;
}
