#include "Raytracing/RayTracer.hpp"
#include <GLFW/glfw3.h>

void RayTracer::setupScence() {
}
void RayTracer::render(shared_ptr<Camera> camera,vec3 intensity,vec3 lightPos, vec3 groundMin, vec3 groundMax,vec3 IOR,float shadow) {
	vec3 coord = camera->getPixel00Pos();
	df_list.BindLocation(2);
	const ShaderProgram& program = use_splat ? splat_program : this->program;
	program.use();
	program.setInt("objects_count", objects_count);
	program.setVec3("eye_coord", camera->position);
	program.setVec3("pixel00Coord", coord);
	program.setVec3("du", camera->du);
	program.setVec3("dv", camera->dv);
	program.setVec3("groundMin", groundMin);
	program.setVec3("groundMax", groundMax);

	program.setVec3("intensity", intensity);
	program.setVec3("lightPos", lightPos);

	program.setInt("window_w", window_width);
	program.setInt("window_h", window_height);

	program.setVec3("EYE_IOR", IOR);
	program.setFloat("shadow_strength", shadow);
	program.setVec3("ambient", ambient);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texture);
	glBindImageTexture(0, texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	glDispatchCompute((window_width + 7)/8, (window_height + 7)/8, 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	display_program.use();
	renderQuad();
}
