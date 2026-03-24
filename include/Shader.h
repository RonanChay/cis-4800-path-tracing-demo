/*
Shader must meet the following criteria:
1) The shaders are in a single file.
2) The shaders are separated by #shader vertex or #shader fragment

example:
#shader vertex
#version 330 core
layout(location = 0) in vec4 position;
void main(){
gl_Position = position;
}

#shader fragment
#version 330 core
out vec4 out_color;
void main() {
out_color = vec4(0.0,1.0,1.0,1.0);
}


Use shader by calling Bind()

Currently only uses vec4, and mat4 uniforms; more can be easily added.
*/

#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp> 
#include <glm/gtc/type_ptr.hpp>
using namespace glm;

// file loading includes
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader {
private:
	GLuint id;
public:
	Shader(const char* file);
	~Shader();

	void Bind() const;
	void Unbind() const;

	// to set a vec4 Uniform. 
	void SetUniform4f(const char* name, glm::vec4 value);

	// to set a vec3 Uniform. 
	void SetUniform3f(const char* name, glm::vec3 value);

	// to set a mat4 Uniform
	void SetUniform4m(const char* name, glm::mat4 value);
	
	void SetUniform1f(const char* name, GLfloat value);
	void SetUniform1i(const char* name, GLint value);
	void SetUniform2f(const char* name, glm::vec2 value);

    void setAlpha(float alpha_value);

private:

	GLint GetUniformLoc(const char* name);
	GLuint CreateShader(const char* vert, const char* frag);
	void ParseShader(const char* file, std::string& vert, std::string& frag);
	GLuint CompileShader(GLuint type, const char* source);
};
