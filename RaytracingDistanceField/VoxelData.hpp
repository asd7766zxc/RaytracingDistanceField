#pragma once
#include "BufferBuilder.hpp"
#include "aabb.hpp"
#include "MathUtilities.hpp"

//will hold a constant limit range (upperbound is restricted by OpenGL)
//a voxel space is defined by an aabb and a resolution 

class VoxelData {
public:
	BinaryImageBuffer3D voxel_buffer;
	aabb voxel_box;
	float voxel_size = 0;
	int sx = 0, sy = 0, sz = 0;
	VoxelData() {}
	VoxelData(aabb rough_box,int resolution) {
		float longest_axis_size = rough_box.axis(rough_box.longestAxis()).size();
		voxel_size = longest_axis_size / resolution;
		sx = (int)std::ceil(rough_box.x.size() / voxel_size) + 1;
		sy = (int)std::ceil(rough_box.y.size() / voxel_size) + 1;
		sz = (int)std::ceil(rough_box.z.size() / voxel_size) + 1;
		voxel_box = aabb(
			interval(rough_box.x.min, rough_box.x.min + sx * voxel_size),
			interval(rough_box.y.min, rough_box.y.min + sy * voxel_size),
			interval(rough_box.z.min, rough_box.z.min + sz * voxel_size)
		);
		voxel_buffer = BinaryImageBuffer3D(sx, sy, (sz + 31) / 32);
	}
	int getVoxelCount() {
		return sx * sy * sz;
	}
	vec3 getVoxelSpaceBoundary() {
		return vec3(sx, sy, sz);
	}
	mat4 getWorldToVoxelMatrix() const {
		return mat4::scale(1.0f / voxel_size) * mat4::trans(-voxel_box.getMinCorner());
	}
	mat4 getVoxelToWorldMatrix() const {
		return mat4::trans(voxel_box.getMinCorner()) * mat4::scale(voxel_size);
	}
	void resetVoxelCells();
};