#pragma once
#include "ShaderProgram.hpp"
#include "Shader.hpp"
#include "DistanceFieldData.hpp"
#include "DFMesh.hpp"
#include "ModelLoader.hpp"
#include "MeshBuilder.hpp"
#include "Camera.hpp"
#include "GameObject.hpp"
#include "Visualization.hpp"

class RayTracer {
public:
	ShaderProgram program;
	ShaderProgram display_program;
	
	ShaderArrayBuffer<DF_Object> df_list;

	int window_width = 0, window_height = 0;
	GLuint texture = 0;

	RayTracer() {
		program = ShaderProgram({
			Shader("raytrace.comp", GL_COMPUTE_SHADER)
		});
		
		display_program = ShaderProgram({
			Shader("raytrace_display.vert", GL_VERTEX_SHADER),
			Shader("raytrace_display.frag", GL_FRAGMENT_SHADER)
		});

		glGenTextures(1, &texture);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	}
	int objects_count = 0;
	void LoadDistanceFieldDatas(std::vector<DF_Object> objs) {
		objects_count = objs.size();
		df_list.BufferData(objs);
	}
	void update_window(int w,int h) {
		window_width = w, window_height = h;
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, window_width, window_height, 0, GL_RGBA,
			GL_FLOAT, NULL);
	}

	Visualization visualizer;

	void setupScence();
	void render(shared_ptr<Camera> camera, vec3 groundMin, vec3 groundMax);

	unsigned int quadVAO = 0;
	unsigned int quadVBO = 0;
	void renderQuad()
	{
		if (quadVAO == 0)
		{
			float quadVertices[] = {
				// positions        // texture Coords
				-1.0f,  1.0f, 0.0f, 0.0f, 0.0f,
				-1.0f, -1.0f, 0.0f, 0.0f, 1.0f,
				 1.0f,  1.0f, 0.0f, 1.0f, 0.0f,
				 1.0f, -1.0f, 0.0f, 1.0f, 1.0f,
			};
			// setup plane VAO
			glGenVertexArrays(1, &quadVAO);
			glGenBuffers(1, &quadVBO);
			glBindVertexArray(quadVAO);
			glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
		}
		glBindVertexArray(quadVAO);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glBindVertexArray(0);
	}


};
