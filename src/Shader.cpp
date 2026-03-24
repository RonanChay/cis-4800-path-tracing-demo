#include "Shader.h"

Shader::Shader(const char* file) : id(0) {

	std::string vertCode, fragCode;

	ParseShader(file, vertCode, fragCode);
	id = CreateShader(vertCode.c_str(), fragCode.c_str());
}
Shader::~Shader() {
	glDeleteProgram(id);
}

void Shader::Bind() const {
	glUseProgram(id);
}

void Shader::Unbind() const {
	glUseProgram(0);
}

void Shader::SetUniform1f(const char* name, GLfloat value) {
    glUniform1f(GetUniformLoc(name), value);
}
void Shader::SetUniform1i(const char* name, GLint value) {
    glUniform1i(GetUniformLoc(name), value);
}
void Shader::SetUniform4f(const char* name, glm::vec4 value) {
	glUniform4fv(GetUniformLoc(name), 1, glm::value_ptr(value));
}
void Shader::SetUniform3f(const char* name, glm::vec3 value) {
	glUniform3fv(GetUniformLoc(name), 1, glm::value_ptr(value));
}
void Shader::SetUniform4m(const char* name, glm::mat4 value) {
	glUniformMatrix4fv(GetUniformLoc(name), 1, GL_FALSE, glm::value_ptr(value));
}

GLint Shader::GetUniformLoc(const char* name) {
	GLint loc = glGetUniformLocation(id, name);
	// if (loc == -1)
	// 	std::cout << "Shader Warning: Uniform " << name << " not found!" << std::endl;

	return loc;
}

GLuint Shader::CompileShader(GLuint type, const char* source) {
	GLuint sid = glCreateShader(type);
	glShaderSource(sid, 1, &source, nullptr);
	glCompileShader(sid);

	//error check
	int errorCode;
	glGetShaderiv(sid, GL_COMPILE_STATUS, &errorCode);
	if (errorCode == GL_FALSE) {
		int length;
		glGetShaderiv(sid, GL_INFO_LOG_LENGTH, &length);
		char* errorMessage = (char*)alloca(length * sizeof(char));
		glGetShaderInfoLog(sid, length, &length, errorMessage);
		std::cout << "Shader Compile Error!\n" << errorMessage << std::endl;
		glDeleteShader(sid);
		return 0;
	}

	return sid;
}

GLuint Shader::CreateShader(const char* vert, const char* frag) {
	// Compile Shaders
	GLuint pid = glCreateProgram();
	GLuint vertShader = CompileShader(GL_VERTEX_SHADER, vert);
	GLuint fragShader = CompileShader(GL_FRAGMENT_SHADER, frag);

	// link shaders
	glAttachShader(pid, vertShader);
	glAttachShader(pid, fragShader);
	glLinkProgram(pid);
	glValidateProgram(pid);

	// garbage cleanup
	glDeleteShader(vertShader);
	glDeleteShader(fragShader);

	return pid;
}

void Shader::ParseShader(const char* file, std::string& vert, std::string& frag) {
    std::ifstream stream(file);
    std::string line;
    std::stringstream ss[2]; // 0 = vert, 1 = frag
    int type = -1; 

    while (getline(stream, line)) {
        if (line.find("#shader") != std::string::npos) {
            if (line.find("vertex") != std::string::npos) type = 0;
            else if (line.find("fragment") != std::string::npos) type = 1;
        } else {
            if (type != -1) ss[type] << line << std::endl;
        }
    }
    vert = ss[0].str();
    frag = ss[1].str();
}

void Shader::setAlpha(float alpha_value) {
    this->Bind();
    this->SetUniform1f("u_alphaColour", alpha_value);
}