#pragma once
#include "ShaderProgram.hpp"
#include "Mesh.hpp"
#include "Camera.hpp"
#include "VoxelData.hpp"
#include <iostream>

class VoxelRenderer {
public:
	ShaderProgram program;
	VoxelRenderer() {
		program = ShaderProgram({
			Shader("voxel_visualize.vert", GL_VERTEX_SHADER),
			Shader("noShade.frag", GL_FRAGMENT_SHADER)
		});
	}
	void draw(shared_ptr<Camera> camera, vec4 solid_color, shared_ptr<Mesh> mesh, VoxelData data) {
		//glDisable(GL_DEPTH_TEST);
		data.voxel_buffer.BindLocation(0);
		program.use();
		program.setMat4("model", data.getVoxelToWorldMatrix());
		program.setMat4("view", camera->view);
		program.setMat4("proj", camera->proj);
		program.setVec3("view_position", camera->position);
		program.setMat4("textureMat", mat4::identity());
		program.setVec4("solid_color", solid_color);
		program.setFloat("voxel_size", data.voxel_size);

		program.setInt("xdim", data.sx);
		program.setInt("ydim", data.sy);
		program.setInt("zdim", data.sz);

		glBindVertexArray(mesh->VAO);
		glDrawArraysInstanced(GL_TRIANGLES, 0, mesh->vertex_count, data.getVoxelCount());
		glBindVertexArray(0);

		//glEnable(GL_DEPTH_TEST);
	}
};