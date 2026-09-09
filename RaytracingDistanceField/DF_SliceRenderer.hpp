#pragma once
#include "ShaderProgram.hpp"
#include "Mesh.hpp"
#include <iostream>

class DF_SliceRenderer {
public:
	ShaderProgram program;
	DF_SliceRenderer() {
		program = ShaderProgram({
			Shader("df_visualize.vert", GL_VERTEX_SHADER),
			Shader("df_visualize.frag", GL_FRAGMENT_SHADER)
			});
	}
	//topleft with (x,y)
	void draw(vec3 bottomleft_corner,float width, float height, float slice,float max_dist) {
		glDisable(GL_DEPTH_TEST);
		glViewport(bottomleft_corner.x, bottomleft_corner.y, width, height);
		program.use();
		program.setFloat("slice", slice);
		program.setFloat("max_dist", max_dist);
		renderQuad();
		glEnable(GL_DEPTH_TEST);
	}
	unsigned int quadVAO = 0;
	unsigned int quadVBO;
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