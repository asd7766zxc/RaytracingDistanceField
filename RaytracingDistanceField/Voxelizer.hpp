#pragma once
#include "Mesh.hpp"
#include "ShaderProgram.hpp"
#include "aabb.hpp"
#include "VoxelData.hpp"

class Voxelizer {
public:
	ShaderProgram transform_p;
	ShaderProgram voxel_surface_p;
	ShaderProgram voxel_solid_p;

	TransformFeedbackBuffer tf_buffer;

	Voxelizer() {
		transform_p = ShaderProgram({
			Shader("voxelize_transform.vert", GL_VERTEX_SHADER)
		},"transformed_position");
		voxel_surface_p = ShaderProgram({
			Shader("voxelize_surface.comp", GL_COMPUTE_SHADER)
		});
		voxel_solid_p = ShaderProgram({
			Shader("voxelize_solid.comp", GL_COMPUTE_SHADER)
		});
	}
	void TransformVertices(mat4 model, shared_ptr<Mesh> mesh) {
		transform_p.use();
		transform_p.setMat4("model", model);

		tf_buffer.AdjustSize(mesh->vertex_count * sizeof(float) * 3);
		tf_buffer.BindLocation(0);

		glBindVertexArray(mesh->VAO);

		glBeginTransformFeedback(GL_TRIANGLES);
		glEnable(GL_RASTERIZER_DISCARD);

		glDrawArrays(GL_TRIANGLES, 0, mesh->vertex_count);

		glEndTransformFeedback();
		glDisable(GL_RASTERIZER_DISCARD);
		glFlush();

		glMemoryBarrier(GL_TRANSFORM_FEEDBACK_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

		glBindVertexArray(0);
	}
	VoxelData buildVoxelData(aabb box,int resolution) {
		VoxelData voxel_data = VoxelData(box, resolution);
		return voxel_data;
	}
	// working in WCS
	VoxelData Voxelize(VoxelData voxel_data,mat4 model,shared_ptr<Mesh> mesh) {
		voxel_data.resetVoxelCells();
		aabb box = voxel_data.voxel_box;

		TransformVertices(voxel_data.getWorldToVoxelMatrix() * model, mesh);

		tf_buffer.BindShaderBufferLocation(0); // buffer 0 for vertex data

		voxel_data.voxel_buffer.BindLocation(1);

		voxel_surface_p.use();
		voxel_surface_p.setVec3("box_max",voxel_data.getVoxelSpaceBoundary());
		voxel_surface_p.dispatchCompute(mesh->vertex_count / 3, 1, 1); // one triangle per invokation

		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		return voxel_data;
	}
};