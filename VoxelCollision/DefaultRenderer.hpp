#pragma once
#pragma
#include "ShaderProgram.hpp"
#include "Mesh.hpp"

class DefaultRenderer {
public:
	ShaderProgram program;
	DefaultRenderer() {
		program = ShaderProgram({
			Shader("defaultShade.vert", GL_VERTEX_SHADER),
			Shader("defaultShade.frag", GL_FRAGMENT_SHADER)
		});
	}
	void draw(shared_ptr<Camera> camera,vec4 solid_color,mat4 transformation,shared_ptr<Mesh> mesh) {
		program.use();
		program.setMat4("model", transformation);
		program.setMat4("view", camera->view);
		program.setMat4("proj", camera->proj);
		program.setVec3("view_position", camera->position);
		program.setMat4("textureMat", mat4::identity());
		program.setVec4("solid_color",solid_color);

		glBindVertexArray(mesh->VAO);
		glDrawArrays(GL_TRIANGLES, 0, mesh->vertex_count);
		glBindVertexArray(0);
	}
};