#pragma once
#include "Helpers.hpp"
#include <glad/glad.h>
//Explicit decide the buffer type for convience to map back to CPU
// vec3 will pad to 16 bytes. 

template<class T>
class ShaderBuffer {
public:
	GLuint ID;
	shared_ptr<T> default_data;
	ShaderBuffer() {
		glGenBuffers(1, &ID);
	}
	void BindLocation(GLuint location) {
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, location, ID);
	}
	void BufferData(GLenum usage = GL_DYNAMIC_READ) {
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ID);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(T), default_data.get(), usage);
	}
	void BufferData(T data, GLenum usage = GL_DYNAMIC_READ) {
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ID);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(T), &data, usage);
	}
	T MapBuffer(GLenum access = GL_READ_ONLY) {
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ID);
		T ret;
		ret = *(T*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, access);
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		return ret;
	}
};
template<class T>
class ShaderArrayBuffer {
public:
	GLuint ID;
	shared_ptr<T> default_data;
	ShaderArrayBuffer() {
		glGenBuffers(1, &ID);
	}
	void setLenght(int length) {
		n = length;
	}
	size_t size() {
		return n * sizeof(T);
	}
	void BindLocation(GLuint location) {
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, location, ID);
	}
	void AdjustSize(GLenum usage = GL_DYNAMIC_READ) {
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ID);
		glBufferData(GL_SHADER_STORAGE_BUFFER, size(), NULL, usage);
	}
	void BufferDataCopy(vector<T> data, GLenum usage = GL_DYNAMIC_READ) {
		n = data.size();
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ID);
		glBufferData(GL_SHADER_STORAGE_BUFFER, size(), data.data(), usage);
	}
	void BufferData(vector<T>& data, GLenum usage = GL_DYNAMIC_READ) {
		n = data.size();
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ID);
		glBufferData(GL_SHADER_STORAGE_BUFFER, size(), data.data(), usage);
	}
	//assume the computeShader will not change the size of the array.
	T* MapBuffer(GLenum access = GL_READ_ONLY) {
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ID);
		T* ret = new T[n];
		T* mapped = (T*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, access);
		std::memcpy(ret, mapped, sizeof(T) * n);
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		return ret;
	}
private:
	int n = 0;
};
class TransformFeedbackBuffer {
public:
	GLuint ID;
	TransformFeedbackBuffer() {
		glGenBuffers(1, &ID);
	}
	void AdjustSize(size_t size, GLenum usage = GL_STATIC_READ) {
		glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, ID);
		glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER, size, NULL, usage);
	}
	void BindLocation(GLuint location) {
		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, location, ID);
	}
	void BindShaderBufferLocation(GLuint location) {
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, location, ID);
	}
	vector<float> MapBuffer(int n,GLenum access = GL_READ_ONLY) {
		vector<float> ret(n);
		glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER,0,n * sizeof(float),  ret.data());
		return ret;
	}
};
class BinaryImageBuffer3D {
public:
	GLuint ID = 0;
	BinaryImageBuffer3D() { // for empty handle
	}
	int sx = 0, sy = 0, sz = 0;
	BinaryImageBuffer3D(int sx,int sy,int sz) : sx(sx),sy(sy),sz(sz) {
		glGenTextures(1, &ID);
		glBindTexture(GL_TEXTURE_3D, ID);
		glTexStorage3D(GL_TEXTURE_3D, 1, GL_R32UI, sx, sy, sz);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}
	void BindLocation(GLuint location) {
		glBindImageTexture(location, ID, 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);
	}
	void BufferData(BYTE* data) {
		glBindTexture(GL_TEXTURE_3D, ID);
		glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, sx, sy, sz, GL_RED_INTEGER, GL_UNSIGNED_BYTE, data);
	}
	void MapBuffer(BYTE* ptr) {
		glBindTexture(GL_TEXTURE_3D, ID);
		glGetTexImage(GL_TEXTURE_3D, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, ptr);
	}
};
//red component float 
class SingleFloatImageBuffer3D {
public:
	GLuint ID = 0;
	GLuint64 handle = 0;
	SingleFloatImageBuffer3D() { // for empty handle
	}
	int sx = 0, sy = 0, sz = 0;
	SingleFloatImageBuffer3D(int sx, int sy, int sz) : sx(sx), sy(sy), sz(sz) {
		glGenTextures(1, &ID);
		glBindTexture(GL_TEXTURE_3D, ID);
		glTexStorage3D(GL_TEXTURE_3D, 1, GL_R32F, sx, sy, sz);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		handle = glGetTextureHandleARB(ID);

	}
	void BindLocation(GLuint location) {
		glBindImageTexture(location, ID, 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32F);
	}
	void BufferData(float* data) {
		glBindTexture(GL_TEXTURE_3D, ID);
		glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, sx, sy, sz, GL_RED, GL_FLOAT, data);
	}
	void Bind() {
		glBindTexture(GL_TEXTURE_3D, ID);
	}
	void MakeResident() { // make buffer ready
		glMakeTextureHandleResidentARB(handle);
	}
	void MapBuffer(float* ptr) {
		glBindTexture(GL_TEXTURE_3D, ID);
		glGetTexImage(GL_TEXTURE_3D, 0, GL_RED, GL_FLOAT, ptr);
	}
};

class RGBA32UIntImageBuffer2D {
public:
	GLuint ID = 0;
	GLuint64 handle = 0;
	RGBA32UIntImageBuffer2D() { // for empty handle
	}
	int sx = 0, sy = 0;
	RGBA32UIntImageBuffer2D(int sx, int sy) : sx(sx), sy(sy) {
		glGenTextures(1, &ID);
		glBindTexture(GL_TEXTURE_2D, ID);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32UI, sx, sy, 0, GL_RGBA_INTEGER, GL_UNSIGNED_INT, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		handle = glGetTextureHandleARB(ID);

	}
	void BindLocation(GLuint location) {
		glBindImageTexture(location, ID, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA32UI);
	}
	void BufferData(float* data) {
		//glBindTexture(GL_TEXTURE_3D, ID);

	}
	void Bind() {
		//glBindTexture(GL_TEXTURE_3D, ID);
	}
	void MakeResident() { // make buffer ready
		glMakeTextureHandleResidentARB(handle);
	}
	void MapBuffer(float* ptr) {
		
	}
	void Clear();
};
class R32UIntImageBuffer2D {
public:
	GLuint ID = 0;
	GLuint64 handle = 0;
	R32UIntImageBuffer2D() { // for empty handle
	}
	int sx = 0, sy = 0;
	R32UIntImageBuffer2D(int sx, int sy) : sx(sx), sy(sy) {
		glGenTextures(1, &ID);
		glBindTexture(GL_TEXTURE_2D, ID);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, sx, sy, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		handle = glGetTextureHandleARB(ID);

	}
	void BindLocation(GLuint location) {
		glBindImageTexture(location, ID, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
	}
	void BufferData(float* data) {
		//glBindTexture(GL_TEXTURE_3D, ID);

	}
	void Bind() {
		//glBindTexture(GL_TEXTURE_3D, ID);
	}
	void MakeResident() { // make buffer ready
		glMakeTextureHandleResidentARB(handle);
	}
	void MapBuffer(uint32_t* ptr) {
		glBindTexture(GL_TEXTURE_2D, ID);
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RED_INTEGER, GL_UNSIGNED_INT,ptr);
	}
	void Clear();
};
class BufferBuilder {
public:
	
};