#pragma once
#include "interval.hpp"
#include "Helpers.hpp"
#include "ray.hpp"

class aabb {
public:
	interval x, y, z;

	aabb() {}
	aabb(interval x, interval y, interval z) : x(x), y(y), z(z) {}
	aabb(aabb& a, aabb& b) : x(a.x, b.x), y(a.y, b.y), z(a.z, b.z) {}
	aabb(vec3 min_corner, vec3 max_corner) {
		adjust(min_corner);
		adjust(max_corner);
	}
	const interval& axis(int i) {
		if (i == 0) return x;
		if (i == 1) return y;
		return z;
	}
	const interval& axis_interval(int i) const {
		if (i == 0) return x;
		if (i == 1) return y;
		return z;
	}
	void adjust(const vec3& v) {
		x.adjust(v.x);
		y.adjust(v.y);
		z.adjust(v.z);
	}
	bool hit(const aabb& other) const {
		return x.intersect(other.x) && y.intersect(other.y) && z.intersect(other.z);
	}
	bool hit(const ray& r, interval& ray_tt) const {
		const vec3& ray_orig = r.origin();
		const vec3& ray_dir = r.direction();

		interval ray_t = ray_tt;
		for (int axis = 0; axis < 3; axis++) {
			const interval& ax = axis_interval(axis);
			const double adinv = 1.0 / ray_dir[axis];

			auto t0 = (ax.min - ray_orig[axis]) * adinv;
			auto t1 = (ax.max - ray_orig[axis]) * adinv;
			if (t0 > t1) std::swap(t0, t1);
			if (t0 > ray_t.min) ray_t.min = t0;
			if (t1 < ray_t.max) ray_t.max = t1;
			if (ray_t.max <= ray_t.min)
				return false;
		}
		ray_tt = ray_t;
		return true;
	}
	void reset() {
		x = interval();
		y = interval();
		z = interval();
	}
	void expand(float scaling) {
		float xs = x.size();
		x.min -= scaling * xs;
		x.max += scaling * xs;

		float ys = y.size();
		y.min -= scaling * ys;
		y.max += scaling * ys;

		float zs = z.size();
		z.min -= scaling * zs;
		z.max += scaling * zs;
	}
	int longestAxis() const {
		float x_size = x.size();
		float y_size = y.size();
		float z_size = z.size();
		if (x_size > y_size && x_size > z_size) return 0;
		else if (y_size > x_size && y_size > z_size) return 1;
		else return 2;
	}
	vec3 getMinCorner() const {
		return vec3(x.min, y.min, z.min);
	}
	vec3 getMaxCorner() const {
		return vec3(x.max, y.max, z.max);
	}
	vec3 size() const {
		return vec3(x.size(),y.size(),z.size());
	}
};