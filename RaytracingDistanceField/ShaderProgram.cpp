#include "ShaderProgram.hpp"
ShaderProgram::ShaderProgram(vector<Shader> shaders) {

	ID = glCreateProgram();
	for (auto shader : shaders) {
		glAttachShader(ID, shader.ID);
	}
	glLinkProgram(ID);
	checkCompileErrors();
}
ShaderProgram::ShaderProgram(vector<Shader> shaders,const char* feedbackVaringName) {

	ID = glCreateProgram();
	for (auto shader : shaders) {
		glAttachShader(ID, shader.ID);
	}
	glTransformFeedbackVaryings(ID, 1, &feedbackVaringName, GL_INTERLEAVED_ATTRIBS); // all to one buffer
	glLinkProgram(ID);
	checkCompileErrors();
}
void ShaderProgram::checkCompileErrors() {
	int success;
	char infoLog[512];
	glGetProgramiv(ID, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(ID, 512, NULL, infoLog);
		std::cout << "Shader Program Linking Error" << infoLog << std::endl;
	}
}
void ShaderProgram::use() const {
	glUseProgram(ID);
}
void ShaderProgram::setFloat(const std::string& name, float v) const {
	glUniform1f(glGetUniformLocation(ID, name.c_str()), v);
}
void ShaderProgram::setVec3(const std::string& name, vec3 v) const {
	glUniform3f(glGetUniformLocation(ID, name.c_str()), v.x, v.y, v.z);
}
void ShaderProgram::setVec4(const std::string& name, vec4 v) const {
	glUniform4f(glGetUniformLocation(ID, name.c_str()), v.x, v.y, v.z, v.w);
}
void ShaderProgram::setInt(const std::string& name, int v) const {
	glUniform1i(glGetUniformLocation(ID, name.c_str()), v);
}
void ShaderProgram::setMat4(const std::string& name, const mat4& m) const {
	glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_TRUE, m.mt);
}
void ShaderProgram::dispatchCompute(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z) const {
	use();
	glDispatchCompute(num_groups_x, num_groups_y, num_groups_z);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}