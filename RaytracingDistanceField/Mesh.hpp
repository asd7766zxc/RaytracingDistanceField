#pragma once
#include "Helpers.hpp"
#include "Vec.hpp"
#include "BufferBuilder.hpp"
#include "aabb.hpp"

struct Vertex {
	vec3 position;
	vec3 normal;
	vec3 texcoord;
};

class Mesh {
public:
	GLuint VAO = 0, VBO = 0;
	ShaderArrayBuffer<Vertex> verticesBuffer;
	std::vector<Vertex> vertices;
	std::vector<float> unstructured_vertices;

	aabb primitive_aabb;

	int vertex_count = -1;
	void genPrimitiveAABB() {
		primitive_aabb.reset();
		for (const auto& v : vertices) {
			primitive_aabb.adjust(v.position);
		}
	}
	void genVertexBufferForCompute() {
		verticesBuffer.BufferData(vertices);
	}
	void genCpuSideCopy(float* _vertices) {
		vertices.reserve(vertex_count);
		for(int i = 0; i < vertex_count; ++i) {
			Vertex v;
			v.position = vec3(_vertices[i * 8 + 0], _vertices[i * 8 + 1], _vertices[i * 8 + 2]);
			v.normal   = vec3(_vertices[i * 8 + 3], _vertices[i * 8 + 4], _vertices[i * 8 + 5]);
			v.texcoord = vec3(_vertices[i * 8 + 6], _vertices[i * 8 + 7], 0);
			vertices.push_back(v);
		}
		unstructured_vertices = std::vector<float>(_vertices, _vertices + vertex_count * 8);
	}
	void genVertexBufferForDraw(float* _vertices, int count) {
		glGenBuffers(1, &VBO);
		glGenVertexArrays(1, &VAO);

		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, count * sizeof(float), _vertices, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0); // position
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(sizeof(GLfloat) * 3)); // normal
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(sizeof(GLfloat) * 6)); // texture coordinates
		glEnableVertexAttribArray(2);
	}
	Mesh() {}
	Mesh(float* _vertices, int count) {
		vertex_count = count / 8;
		genVertexBufferForDraw(_vertices, count);
		genCpuSideCopy(_vertices);
		genPrimitiveAABB();
	}
	Mesh(float* _vertices, int size, int count) { // to adapt old interface
		vertex_count = count;
		genVertexBufferForDraw(_vertices, count * 8);
		genCpuSideCopy(_vertices);
		genPrimitiveAABB();
	}
};