#pragma once
#include "ShaderProgram.hpp"
#include "Mesh.hpp"
#include "Camera.hpp"

class VisualRenderer {
public:
	ShaderProgram program;
	VisualRenderer() {
		program = ShaderProgram({
			Shader("defaultShade.vert", GL_VERTEX_SHADER),
			Shader("noShade.frag", GL_FRAGMENT_SHADER)
			});
	}
	void draw(shared_ptr<Camera> camera, vec4 solid_color, mat4 transformation, shared_ptr<Mesh> mesh) {
		program.use();
		program.setMat4("model", transformation);
		program.setMat4("view", camera->view);
		program.setMat4("proj", camera->proj);
		program.setVec3("view_position", camera->position);
		program.setMat4("textureMat", mat4::identity());
		program.setVec4("solid_color", solid_color);

		glBindVertexArray(mesh->VAO);
		glDrawArrays(GL_TRIANGLES, 0, mesh->vertex_count);
		glBindVertexArray(0);
	}
};