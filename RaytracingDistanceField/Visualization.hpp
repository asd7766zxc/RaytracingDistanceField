#pragma once
#include "VoxelRenderer.hpp"
#include "ShaderProgram.hpp"
#include "Camera.hpp"
#include "aabb.hpp"
#include "VisualRenderer.hpp"
#include "MeshBuilder.hpp"
#include "VoxelData.hpp"
#include "DF_SliceRenderer.hpp"
#include "DistanceFieldData.hpp"
//#include "Renderer2D.hpp"

class Visualization {
public:
	VisualRenderer render;
	VoxelRenderer voxel_render;
	DF_SliceRenderer df_slice_renderer;

	static void _drawAABB(shared_ptr<Camera> camera,VisualRenderer render,aabb box) {
		auto model = mat4::trans(box.getMinCorner()) \
			* mat4::scale(vec3(box.x.size(), box.y.size(), box.z.size())) \
			* mat4::trans(0.5);
		render.draw(camera, vec4(0, 1, 0, 0.2), model, MeshBuilder::Cube());
	}
	void drawAABB(shared_ptr<Camera> camera, aabb box) {
		_drawAABB(camera,render,box);
	}
	static void _drawDF(DF_SliceRenderer renderer,DistanceFieldData df_data,float slice = .5) {
		df_data.df_buffer.Bind();
		renderer.draw(vec3(0, 0, 0), 256, 256, slice, df_data.longest_axis_length * df_data.df_size);
	}
	void drawDF(DistanceFieldData df_data, float slice = 0.5f) {
		_drawDF(df_slice_renderer, df_data,slice);
	}
	static void _drawPoint(shared_ptr<Camera> camera, VisualRenderer render, vec3 point) {
		auto model = mat4::trans(point) \
			* mat4::scale(0.02);
		render.draw(camera, vec4(1, 1, 0, 0.5), model, MeshBuilder::Sphere(10));
	}
	void drawPoint(shared_ptr<Camera> camera, vec3 point) {
		_drawPoint(camera, render, point);
	}
	static void _drawVoxel(shared_ptr<Camera> camera, VoxelRenderer render, VoxelData data) {
		render.draw(camera, vec4(0, 1, 0, 0.3),MeshBuilder::Cube(), data);
	}
	void drawVoxel(shared_ptr<Camera> camera, VoxelData data) {
		_drawVoxel(camera, voxel_render, data);
	}
};