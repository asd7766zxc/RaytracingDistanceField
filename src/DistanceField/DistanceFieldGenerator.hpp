#pragma once
#include "ShaderPipelines/ShaderProgram.hpp"
#include "DistanceField/DistanceFieldData.hpp"
#include "Mesh/Mesh.hpp"
#include <chrono>

class DistanceFieldGenerator {
public:
	ShaderProgram naive_program;
	ShaderProgram transform_p;
	TransformFeedbackBuffer tf_buffer;

	DistanceFieldGenerator() {
		naive_program = ShaderProgram({
			Shader("visualization/df_naive.comp", GL_COMPUTE_SHADER)
			});
		transform_p = ShaderProgram({
			Shader("visualization/df_transform.vert", GL_VERTEX_SHADER)
			}, "transformed_position");
	};
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

	void generateDistanceField(DistanceFieldData df_data, shared_ptr<Mesh> mesh) {
		if(read_distace_field_cache(df_data,mesh)) return;

		auto start = std::chrono::steady_clock::now();
		 TransformVertices(mat4::trans(df_data.df_box.size() * 0.5) * mat4::scale(0.75) * mat4::trans(-df_data.df_box.size()*0.5-df_data.df_box.getMinCorner()), mesh);
		//TransformVertices(mat4::trans(-df_data.df_box.getMinCorner()), mesh);
		naive_program.use();
		naive_program.setFloat("df_size", df_data.df_size);
		naive_program.setInt("vertex_count", mesh->vertex_count);
		naive_program.setInt("sx", df_data.sx);
		naive_program.setInt("sy", df_data.sy);
		naive_program.setInt("sz", df_data.sz);

		tf_buffer.BindShaderBufferLocation(0);
		df_data.df_buffer.BindLocation(1);

		glDispatchCompute((df_data.sx + 7) / 8, (df_data.sy + 7) / 8, (df_data.sz + 7) / 8);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
		auto end = std::chrono::steady_clock::now();
		
		glFinish();

		std::cout << std::endl << "distance field calculated for model: " << mesh->VAO << " using " << (std::chrono::duration_cast<std::chrono::seconds>(end - start)) << " seconds." << std::endl;
		write_distance_field_cache(df_data,mesh);
	}
	
	bool read_distace_field_cache(DistanceFieldData df_data, shared_ptr<Mesh> mesh);
	void write_distance_field_cache(DistanceFieldData df_data, shared_ptr<Mesh> mesh);
	//TODO: hash the mesh for storing df cache
};