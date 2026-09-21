#pragma once
#include "Mesh.hpp"
#include "Voxelizer.hpp"
#include "DistanceFieldData.hpp"
#include "DistanceFieldGenerator.hpp"
// move the mesh's gravity center to LCS's center 
class DFMesh : public Mesh {
public:
	DistanceFieldData df_data;
	void updateMesh(float* _vertices, int count) {
		vertex_count = count / 8;
		genVertexBufferForDraw(_vertices, count);
		genCpuSideCopy(_vertices);
		genPrimitiveAABB();
	}
	DFMesh() {}
	DFMesh(DistanceFieldGenerator df_generator,shared_ptr<Mesh> mesh,mat4 model, float scaling = 1.0f,int resolution = 100) {
		auto vertices = mesh->unstructured_vertices;
		for (int i = 0; i < mesh->vertex_count; ++i) {
			vec3 tmp(vertices[i * 8 + 0], vertices[i * 8 + 1], vertices[i * 8 + 2]);
			tmp *= scaling;
			tmp = (model * vec4(tmp,1)).toVec3();
			vertices[i * 8 + 0] = tmp.x;
			vertices[i * 8 + 1] = tmp.y;
			vertices[i * 8 + 2] = tmp.z;
		}
		updateMesh(vertices.data(), vertices.size());
		df_data = DistanceFieldData(primitive_aabb, resolution);
		df_generator.generateDistanceField(df_data, make_shared<Mesh>(*this));
	}
};