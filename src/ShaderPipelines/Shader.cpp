#include "ShaderPipelines/Shader.hpp"


Shader::Shader(const char* source, GLuint shader_type) {
	std::string shader_code = resolveInclude(loadShaderSource(source));

	ID = glCreateShader(shader_type);
	const char* shader_code_cstr = shader_code.c_str();
	glShaderSource(ID, 1, &shader_code_cstr, NULL);
	glCompileShader(ID);
	checkCompileErrors();
}
std::string Shader::loadShaderSource(const std::string& path) {
	std::ifstream shader_file;
	shader_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	std::string shader_code;

	try {
		shader_file.open(SHADER_DIR + path);
		std::stringstream ss;
		ss << shader_file.rdbuf();
		shader_file.close();
		shader_code = ss.str();

	}
	catch (std::ifstream::failure& e) {
		std::cout << "Fail to read shader source : " << e.what() << std::endl;
	}
	return shader_code;
}
std::string Shader::resolveInclude(const std::string& code) {
	int n = code.size();
	std::string result = "";
	for (int i = 0; i < n; ++i) {
		if (code[i] == '#' && code.substr(i, 8) == "#include") {
			int j = i + 8;
			while (j < n && code[j] == ' ') ++j;
			if (j < n && code[j] == '"') {
				++j;
				std::string path;
				while (j < n && code[j] != '"') path += code[j++];
				result += resolveInclude(loadShaderSource(path));
				i = j + 1;
			}
		}
		else {
			result += code[i];
		}
	}
	return result;
}

void Shader::checkCompileErrors() {
	int success;
	char infoLog[512];
	glGetShaderiv(ID, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(ID, 512, NULL, infoLog);
		std::cout << "Shader Compilation Error" << infoLog << std::endl;
	}
}