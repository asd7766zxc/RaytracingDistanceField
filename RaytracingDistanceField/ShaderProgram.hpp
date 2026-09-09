#pragma once
#include <glad/glad.h>
#include "Shader.hpp"
#include "Helpers.hpp"
#include "Vec.hpp"

class ShaderProgram {
public:
	unsigned int ID = 0;
	ShaderProgram() {};
	ShaderProgram(vector<Shader> shaders);
	ShaderProgram(vector<Shader> shaders, const char* feedbackVaringName);
	void checkCompileErrors();
	void use() const;
	void setInt(const std::string& name, int v) const;
	void setFloat(const std::string& name, float v) const;
	void setVec3(const std::string& name, vec3 v) const;
	void setVec4(const std::string& name, vec4 v) const;
	void setMat4(const std::string& name, const mat4& m) const;
	void dispatchCompute(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z) const;
};