#pragma once
#include "ShaderProgram.hpp"
#include "BufferBuilder.hpp"
#include <GLFW/glfw3.h>
#include "GameObject.hpp"

class PolygonSplatting {
public:
	ShaderProgram polygonsplat;
	ShaderProgram rasterize_splat;
	GLuint caustic_map = 0;
	int map_w = 0, map_h = 0;
	// R / G / B
	vec3 refract_indices = vec3(1.50f, 1.40f, 1.30f);
	ShaderArrayBuffer<DF_Object> df_list;
	struct splat_vertex {
		vec4 pos;
		vec4 path;
	};
	ShaderArrayBuffer<splat_vertex> splatted_list;

	float energy_scale = 1350000.0f;
	float min_area_px = 0.05f;
	float max_area_ratio = 0.05f;          

	PolygonSplatting(int gWidth, int gHeight) {
		polygonsplat = ShaderProgram({
			Shader("polygonsplat.comp", GL_COMPUTE_SHADER)
			});
		rasterize_splat = ShaderProgram({
			Shader("rasterize_splat.vert", GL_VERTEX_SHADER),
			Shader("rasterize_splat.frag", GL_FRAGMENT_SHADER)
			});
		map_w = gWidth, map_h = gHeight;
		glGenTextures(1, &caustic_map);
		glBindTexture(GL_TEXTURE_2D_ARRAY, caustic_map);
		glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_R32UI, map_w, map_h, 3);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glGenVertexArrays(1, &emptyVAO);

		glGenFramebuffers(1, &fbo); 
	}
	int objects_count = 0;
	void LoadDistanceFieldDatas(std::vector<DF_Object> objs) {
		objects_count = objs.size();
		df_list.BufferData(objs);
	}
	void render(int grid_size, vec3 intensity, vec3 lightPos, vec3 groundMin, vec3 groundMax) {
		if (grid_size < 2) return;

		
		if (grid_size != allocated_grid) {
			splatted_list.setLenght(grid_size * grid_size);
			splatted_list.AdjustSize();
			allocated_grid = grid_size;
		}
		df_list.BindLocation(2);
		splatted_list.BindLocation(7);

		GLint prev_viewport[4];
		glGetIntegerv(GL_VIEWPORT, prev_viewport);
		GLboolean depth_was_on = glIsEnabled(GL_DEPTH_TEST);
		GLboolean blend_was_on = glIsEnabled(GL_BLEND);

		glBindImageTexture(6, caustic_map, 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI); // all 3 layers

		// clear the 3 RGB layers
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		for (int layer = 0; layer < 3; ++layer) {
			glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, caustic_map, 0, layer);
			glDrawBuffer(GL_COLOR_ATTACHMENT0);
			const GLuint zero[4] = { 0, 0, 0, 0 };
			glClearBufferuiv(GL_COLOR, 0, zero);
		}
		glDrawBuffer(GL_NONE);
		glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		glViewport(0, 0, map_w, map_h);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);

		const float iors[3] = { refract_indices.x, refract_indices.y, refract_indices.z };
		const float powers[3] = { intensity.x, intensity.y, intensity.z };
		for (int channel = 0; channel < 3; ++channel) {
		polygonsplat.use();
		polygonsplat.setInt("objects_count", objects_count);
		polygonsplat.setInt("grid_size", grid_size);
		polygonsplat.setVec3("intensity", intensity);
		polygonsplat.setVec3("lightPos", lightPos);
		polygonsplat.setVec3("groundMin", groundMin);
		polygonsplat.setVec3("groundMax", groundMax);
		polygonsplat.setFloat("refract_index", iors[channel]);

		glDispatchCompute((grid_size + 7) / 8, (grid_size + 7) / 8, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		rasterize_splat.use();
		rasterize_splat.setInt("grid_size", grid_size);
		rasterize_splat.setInt("channel", channel);
		rasterize_splat.setVec3("groundMin", groundMin);
		rasterize_splat.setVec3("groundMax", groundMax);
		rasterize_splat.setFloat("intensity", powers[channel]);
		rasterize_splat.setFloat("energy_scale", energy_scale);
		rasterize_splat.setFloat("min_area_px", min_area_px);
		rasterize_splat.setFloat("max_area_px", max_area_ratio * map_w * map_h);
		glUniform2f(glGetUniformLocation(rasterize_splat.ID, "map_size"), (float)map_w, (float)map_h);

		glBindVertexArray(emptyVAO);
		int cells = grid_size - 1;
		glDrawArrays(GL_TRIANGLES, 0, cells * cells * 6);
		glBindVertexArray(0);

		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
		}

		// restore state
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(prev_viewport[0], prev_viewport[1], prev_viewport[2], prev_viewport[3]);
		if (depth_was_on) glEnable(GL_DEPTH_TEST);
		if (blend_was_on) glEnable(GL_BLEND);
	}
private:
	GLuint emptyVAO = 0;
	GLuint fbo = 0;
	int allocated_grid = 0;
};
