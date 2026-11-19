#pragma once
#include "ShaderPipelines/ShaderProgram.hpp"
#include "ShaderPipelines/BufferBuilder.hpp"
#include <GLFW/glfw3.h>
#include "GameObject.hpp"

class PolygonSplatting {
public:
	ShaderProgram polygonsplat;
	ShaderProgram rasterize_splat;
	GLuint caustic_map = 0;
	int map_w = 0, map_h = 0;
	// R / G / B
	ShaderArrayBuffer<DF_Object> df_list;
	struct splat_vertex {
		vec4 pos;
		vec4 path;
	};
	ShaderArrayBuffer<splat_vertex> splatted_list;

	float energy_scale = 100000.0f;
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
	static float cie_g(float x, float mu, float s1, float s2) { float t = (x - mu) / (x < mu ? s1 : s2); return std::exp(-0.5f * t * t); }
	static vec3 wavelengthToRGB(float nm) {
    float X = 1.056f*cie_g(nm,599.8f,37.9f,31.0f) + 0.362f*cie_g(nm,442.0f,16.0f,26.7f) - 0.065f*cie_g(nm,501.1f,20.4f,26.2f);
    float Y = 0.821f*cie_g(nm,568.8f,46.9f,40.5f) + 0.286f*cie_g(nm,530.9f,16.3f,31.1f);
    float Z = 1.217f*cie_g(nm,437.0f,11.8f,36.0f) + 0.681f*cie_g(nm,459.0f,26.0f,13.8f);
    return vec3(std::max( 3.2406f*X - 1.5372f*Y - 0.4986f*Z, 0.f) / 0.4403f,
                std::max(-0.9689f*X + 1.8758f*Y + 0.0415f*Z, 0.f) / 0.2884f,
                std::max( 0.0557f*X - 0.2040f*Y + 1.0570f*Z, 0.f) / 0.2732f);
	}
	void render(std::vector<float> iors,vec3 iorI,int grid_size, vec3 intensity, vec3 lightPos, vec3 groundMin, vec3 groundMax,bool point_mode,vec3 spread) {
		if (grid_size < 2) return;

		
		if (grid_size != allocated_grid) {
			splatted_list.setLenght(grid_size * grid_size);
			splatted_list.AdjustSize();
			allocated_grid = grid_size;
		}

		std::vector<vec3> w(iors.size()); vec3 sum(0.0f);
		for (size_t i = 0; i < iors.size(); ++i) { w[i] = wavelengthToRGB(380.f + 400.f * iors[i]); sum = sum + w[i]; }
		for (auto& x : w) x = vec3(sum.x > 0 ? x.x / sum.x : 0, sum.y > 0 ? x.y / sum.y : 0, sum.z > 0 ? x.z / sum.z : 0);

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

		const float powers[3] = { intensity.x, intensity.y, intensity.z };
		for (int channel = 0; channel < iors.size(); ++channel) {
		polygonsplat.use();
		polygonsplat.setInt("objects_count", objects_count);
		polygonsplat.setInt("grid_size", grid_size);
		polygonsplat.setVec3("intensity", intensity);
		polygonsplat.setVec3("lightPos", lightPos);
		polygonsplat.setVec3("groundMin", groundMin);
		polygonsplat.setVec3("groundMax", groundMax);
		polygonsplat.setFloat("refract_index", iorI.y + (iorI.x - iorI.y) * iors[channel]);
		polygonsplat.setInt("point_mode", point_mode);
		polygonsplat.setVec3("spread", spread);

		glDispatchCompute((grid_size + 7) / 8, (grid_size + 7) / 8, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		rasterize_splat.use();
		rasterize_splat.setInt("grid_size", grid_size);
		rasterize_splat.setInt("channel", channel);
		rasterize_splat.setVec3("groundMin", groundMin);
		rasterize_splat.setVec3("groundMax", groundMax);
		rasterize_splat.setFloat("intensity", intensity.x);
		rasterize_splat.setFloat("energy_scale", energy_scale);
		rasterize_splat.setFloat("min_area_px", min_area_px);
		rasterize_splat.setFloat("max_area_px", max_area_ratio * map_w * map_h);
		rasterize_splat.setVec3("channel_weight", w[channel]);
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
