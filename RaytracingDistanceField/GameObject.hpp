#pragma once
#include "Mesh.hpp"
#include "DistanceFieldData.hpp"
#include "DFMesh.hpp"

struct DF_Object {
	mat4 model;
	uint64_t handle;
	uint64_t pad1;
	vec3 df_scale;
	vec4 material;
	vec4 brdf;
	ivec4 type;
	float df_size;
};

class GameObject {
public:
	shared_ptr<DFMesh> df_mesh;

	vec3 position;
	quat orientation = quat(1, 0, 0, 0);
	
	vec4 material;
	vec4 brdf;
	ivec4 type;

	mat4 getModelMatrix() const {
		return mat4::trans(position) * mat4::quat(orientation);
	}
	mat4 getWorldToModel() const {
		return mat4::quat(orientation).transposed() * mat4::trans(-position);
	}
	void prepareDFData() const {
		df_mesh->df_data.df_buffer.MakeResident();
	}
	DF_Object getDFOjbect() const {
		auto& data = df_mesh->df_data;
		return {
			.model = (mat4::trans(data.df_box.size() * 0.5) * getWorldToModel()).transposed(),
			.handle	= data.df_buffer.handle,
			.df_scale = data.getDistanceFieldSpaceBoundary() * data.df_size,
			.material = material,
			.brdf = brdf,
			.type = type,
			.df_size = data.df_size,
		};
	}
};