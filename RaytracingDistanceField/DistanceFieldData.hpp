#pragma once
#include "BufferBuilder.hpp"
#include "aabb.hpp"
#include "MathUtilities.hpp"

//will hold a constant limit range (upperbound is restricted by OpenGL)

class DistanceFieldData {
public:
	SingleFloatImageBuffer3D df_buffer;
	aabb df_box;
	float df_size = 0;
	float longest_axis_length = 0;
	int sx = 0, sy = 0, sz = 0;
	DistanceFieldData() {}
	DistanceFieldData(aabb rough_box, int resolution) {
		float longest_axis_size = rough_box.axis(rough_box.longestAxis()).size();
		df_size = longest_axis_size / resolution;
		sx = (int)std::ceil(rough_box.x.size() / df_size) + 1;
		sy = (int)std::ceil(rough_box.y.size() / df_size) + 1;
		sz = (int)std::ceil(rough_box.z.size() / df_size) + 1;
		df_box = aabb(
			interval(rough_box.x.min, rough_box.x.min + sx * df_size),
			interval(rough_box.y.min, rough_box.y.min + sy * df_size),
			interval(rough_box.z.min, rough_box.z.min + sz * df_size)
		);
		df_buffer = SingleFloatImageBuffer3D(sx, sy, sz);
		longest_axis_length = std::max({ sx, sy, sz });
	}
	int getVoxelCount() {
		return sx * sy * sz;
	}
	vec3 getDistanceFieldSpaceBoundary() {
		return vec3(sx, sy, sz);
	}
	mat4 getModelToDFMatrix() const {
		return mat4::trans(-df_box.getMinCorner());
	}
	mat4 getDFToModelMatrix() const {
		//return mat4::trans(voxel_box.getMinCorner()) * mat4::scale(voxel_size);
	}
};