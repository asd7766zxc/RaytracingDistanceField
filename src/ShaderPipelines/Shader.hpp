#pragma once

#include <glad/glad.h>
#include <string>


//Shader Wrapper Class
class Shader {
public:
	unsigned int ID;
	Shader(const char* source, GLuint shader_type);
	std::string loadShaderSource(const std::string& path);
	std::string resolveInclude(const std::string& code);
	void checkCompileErrors();
};
