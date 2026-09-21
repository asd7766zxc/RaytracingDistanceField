#pragma once
#include "ShaderProgram.hpp"
#include "BufferBuilder.hpp"
#include <GLFW/glfw3.h>

class PhotonTracer {
public:
	ShaderProgram photontrace;
	ShaderProgram clear_texture;
	ShaderProgram guassian_blur;
	ShaderProgram temporal_accumulation;

	R32UIntImageBuffer2D caustic_map_r;
	R32UIntImageBuffer2D caustic_map_g;
	R32UIntImageBuffer2D caustic_map_b;

	R32UIntImageBuffer2D caustic_map_r_blurred;
	R32UIntImageBuffer2D caustic_map_g_blurred;
	R32UIntImageBuffer2D caustic_map_b_blurred;


	ShaderArrayBuffer<DF_Object> df_list;

	PhotonTracer(int gWidth,int gHeight) {
		photontrace = ShaderProgram({
			Shader("photontrace.comp", GL_COMPUTE_SHADER)
		});
		clear_texture = ShaderProgram({
			Shader("clear_texture.comp", GL_COMPUTE_SHADER)
		});
		guassian_blur = ShaderProgram({
			Shader("guassian_blur.comp", GL_COMPUTE_SHADER)
		});
		temporal_accumulation = ShaderProgram({
			Shader("temporal_accumulation.comp", GL_COMPUTE_SHADER)
		});
		caustic_map_r = R32UIntImageBuffer2D(gWidth, gHeight);
		caustic_map_g = R32UIntImageBuffer2D(gWidth, gHeight);
		caustic_map_b = R32UIntImageBuffer2D(gWidth, gHeight);

		caustic_map_r_blurred = R32UIntImageBuffer2D(gWidth, gHeight);
		caustic_map_g_blurred = R32UIntImageBuffer2D(gWidth, gHeight);
		caustic_map_b_blurred = R32UIntImageBuffer2D(gWidth, gHeight);
	}
	int objects_count = 0;
	void LoadDistanceFieldDatas(std::vector<DF_Object> objs) {
		objects_count = objs.size();
		df_list.BufferData(objs);
	}
	void render(int photon_count,vec3 intensity,vec3 lightPos,vec3 groundMin,vec3 groundMax) {
		caustic_map_r.BindLocation(3);
		caustic_map_g.BindLocation(4);
		caustic_map_b.BindLocation(5);

		clear_texture.use();
		glDispatchCompute((caustic_map_r.sx + 15) / 16, (caustic_map_r.sy + 15) / 16, 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		photontrace.use();
		photontrace.setInt("objects_count", objects_count);
		photontrace.setInt("photon_count", photon_count);
		photontrace.setVec3("intensity", intensity);
		photontrace.setVec3("lightPos", lightPos);
		photontrace.setVec3("groundMin", groundMin);
		photontrace.setVec3("groundMax", groundMax);
		photontrace.setFloat("frame_seed",glfwGetTime());

		glDispatchCompute((photon_count + 63) / 64, 1, 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		caustic_map_r.BindLocation(0);
		caustic_map_g.BindLocation(1);
		caustic_map_b.BindLocation(2);

		caustic_map_r_blurred.BindLocation(3);
		caustic_map_g_blurred.BindLocation(4);
		caustic_map_b_blurred.BindLocation(5);

		guassian_blur.use();

		glDispatchCompute((caustic_map_r.sx + 15) / 16, (caustic_map_r.sy + 15) / 16, 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		temporal_accumulation.use();
		glDispatchCompute((caustic_map_r.sx + 15) / 16, (caustic_map_r.sy + 15) / 16, 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	}
};
