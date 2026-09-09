#include "Mesh.hpp"
#include "DistanceFieldData.hpp"
class GameObject {
public:
	shared_ptr<Mesh> mesh;
	shared_ptr<DistanceFieldData> df_data;

	vec3 scale = vec3(1);
	vec3 position;
	quat orientation = quat(1, 0, 0, 0);

	mat4 getModelMatrix() const {
		return mat4::trans(position) * mat4::quat(orientation) * mat4::scale(scale);
	}
};