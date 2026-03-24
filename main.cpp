// main.cpp
// Entry point: window creation, OpenGL context, render loop, and framebuffer management.

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdlib>

// ─────────────────────────────────────────────────────────────────────────────
// Constants
// ─────────────────────────────────────────────────────────────────────────────

static const int   WINDOW_WIDTH            = 1280;
static const int   WINDOW_HEIGHT           = 720;
static const char* WINDOW_TITLE            = "OpenGL Path Tracer";
static const int   MAX_ACCUMULATION_FRAMES = 4096;

// ─────────────────────────────────────────────────────────────────────────────
// Shader loading helpers
// ─────────────────────────────────────────────────────────────────────────────

static std::string readFileToString(const std::string& filePath)
{
    std::ifstream fileStream(filePath);
    if (!fileStream.is_open()) {
        std::cerr << "[ERROR] Cannot open shader file: " << filePath << "\n";
        std::exit(EXIT_FAILURE);
    }
    std::ostringstream buffer;
    buffer << fileStream.rdbuf();
    return buffer.str();
}

static GLuint compileShader(GLenum shaderType, const std::string& sourceCode)
{
    GLuint shaderHandle = glCreateShader(shaderType);
    const char* rawSource = sourceCode.c_str();
    glShaderSource(shaderHandle, 1, &rawSource, nullptr);
    glCompileShader(shaderHandle);

    GLint compilationSuccess = 0;
    glGetShaderiv(shaderHandle, GL_COMPILE_STATUS, &compilationSuccess);
    if (!compilationSuccess) {
        char infoLog[1024];
        glGetShaderInfoLog(shaderHandle, sizeof(infoLog), nullptr, infoLog);
        std::cerr << "[ERROR] Shader compilation failed:\n" << infoLog << "\n";
        std::exit(EXIT_FAILURE);
    }
    return shaderHandle;
}

static GLuint linkShaderProgram(GLuint vertexShaderHandle, GLuint fragmentShaderHandle)
{
    GLuint programHandle = glCreateProgram();
    glAttachShader(programHandle, vertexShaderHandle);
    glAttachShader(programHandle, fragmentShaderHandle);
    glLinkProgram(programHandle);

    GLint linkSuccess = 0;
    glGetProgramiv(programHandle, GL_LINK_STATUS, &linkSuccess);
    if (!linkSuccess) {
        char infoLog[1024];
        glGetProgramInfoLog(programHandle, sizeof(infoLog), nullptr, infoLog);
        std::cerr << "[ERROR] Shader program link failed:\n" << infoLog << "\n";
        std::exit(EXIT_FAILURE);
    }
    return programHandle;
}

static GLuint buildShaderProgram(const std::string& vertexPath, const std::string& fragmentPath)
{
    std::string vertexSource   = readFileToString(vertexPath);
    std::string fragmentSource = readFileToString(fragmentPath);

    GLuint vertexShader   = compileShader(GL_VERTEX_SHADER,   vertexSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
    GLuint programHandle  = linkShaderProgram(vertexShader, fragmentShader);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return programHandle;
}

// ─────────────────────────────────────────────────────────────────────────────
// Fullscreen quad geometry
// ─────────────────────────────────────────────────────────────────────────────

static GLuint createFullscreenQuadVAO()
{
    // Two triangles covering NDC [-1, 1] in both axes.
    // Each vertex: (posX, posY)
    float quadVertices[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
        -1.0f,  1.0f,
         1.0f,  1.0f,
    };

    GLuint vertexArrayObject  = 0;
    GLuint vertexBufferObject = 0;

    glGenVertexArrays(1, &vertexArrayObject);
    glGenBuffers(1, &vertexBufferObject);

    glBindVertexArray(vertexArrayObject);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObject);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

    glBindVertexArray(0);
    return vertexArrayObject;
}

// ─────────────────────────────────────────────────────────────────────────────
// Accumulation framebuffer pair
// ─────────────────────────────────────────────────────────────────────────────

struct AccumulationBuffer {
    GLuint framebufferHandle;
    GLuint colorTextureHandle;
};

static AccumulationBuffer createAccumulationBuffer(int textureWidth, int textureHeight)
{
    AccumulationBuffer accumulationBuffer = {};

    glGenFramebuffers(1, &accumulationBuffer.framebufferHandle);
    glBindFramebuffer(GL_FRAMEBUFFER, accumulationBuffer.framebufferHandle);

    glGenTextures(1, &accumulationBuffer.colorTextureHandle);
    glBindTexture(GL_TEXTURE_2D, accumulationBuffer.colorTextureHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F,
                 textureWidth, textureHeight, 0,
                 GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, accumulationBuffer.colorTextureHandle, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "[ERROR] Framebuffer is not complete.\n";
        std::exit(EXIT_FAILURE);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return accumulationBuffer;
}

// ─────────────────────────────────────────────────────────────────────────────
// Camera state
// ─────────────────────────────────────────────────────────────────────────────

struct CameraState {
    glm::vec3 position = glm::vec3(0.0f, 1.0f, 5.0f);
    float     focalLength = 1.5f;
};

// ─────────────────────────────────────────────────────────────────────────────
// Main
// ─────────────────────────────────────────────────────────────────────────────

int main()
{
    // ── GLFW init ────────────────────────────────────────────────────────────
    if (!glfwInit()) {
        std::cerr << "[ERROR] Failed to initialise GLFW.\n";
        return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* windowHandle = glfwCreateWindow(
        WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, nullptr, nullptr);
    if (!windowHandle) {
        std::cerr << "[ERROR] Failed to create GLFW window.\n";
        glfwTerminate();
        return EXIT_FAILURE;
    }
    glfwMakeContextCurrent(windowHandle);

    // ── GLEW init ────────────────────────────────────────────────────────────
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "[ERROR] Failed to initialise GLEW.\n";
        return EXIT_FAILURE;
    }

    // ── Shaders ──────────────────────────────────────────────────────────────
    GLuint pathTraceProgram = buildShaderProgram("shaders/fullscreen_quad.vert",
                                                 "shaders/path_trace.frag");
    GLuint tonemapProgram   = buildShaderProgram("shaders/fullscreen_quad.vert",
                                                 "shaders/tonemap.frag");

    // ── Geometry ─────────────────────────────────────────────────────────────
    GLuint fullscreenQuadVAO = createFullscreenQuadVAO();

    // ── Ping-pong accumulation buffers ───────────────────────────────────────
    AccumulationBuffer accumulationBufferA = createAccumulationBuffer(WINDOW_WIDTH, WINDOW_HEIGHT);
    AccumulationBuffer accumulationBufferB = createAccumulationBuffer(WINDOW_WIDTH, WINDOW_HEIGHT);

    // Alternate each frame: write into currentWriteBuffer, read from currentReadBuffer.
    AccumulationBuffer* currentWriteBuffer = &accumulationBufferA;
    AccumulationBuffer* currentReadBuffer  = &accumulationBufferB;

    // ── Uniform locations: path trace pass ───────────────────────────────────
    GLint uniformFrameCount      = glGetUniformLocation(pathTraceProgram, "uniformFrameCount");
    GLint uniformElapsedTime     = glGetUniformLocation(pathTraceProgram, "uniformElapsedTime");
    GLint uniformCameraPosition  = glGetUniformLocation(pathTraceProgram, "uniformCameraPosition");
    GLint uniformAccumTexture    = glGetUniformLocation(pathTraceProgram, "uniformAccumTexture");
    GLint uniformWindowWidth     = glGetUniformLocation(pathTraceProgram, "uniformWindowWidth");
    GLint uniformWindowHeight    = glGetUniformLocation(pathTraceProgram, "uniformWindowHeight");

    // ── Uniform locations: tonemap pass ──────────────────────────────────────
    GLint uniformTonemapTexture  = glGetUniformLocation(tonemapProgram, "uniformAccumTexture");

    // ── Camera ───────────────────────────────────────────────────────────────
    CameraState camera;

    // ── Render loop ──────────────────────────────────────────────────────────
    int accumulatedFrameCount = 0;

    while (!glfwWindowShouldClose(windowHandle)
           && accumulatedFrameCount < MAX_ACCUMULATION_FRAMES)
    {
        float elapsedSeconds = static_cast<float>(glfwGetTime());

        // ── Pass 1: path trace into currentWriteBuffer ────────────────────
        glBindFramebuffer(GL_FRAMEBUFFER, currentWriteBuffer->framebufferHandle);
        glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(pathTraceProgram);
        glUniform1i(uniformFrameCount,     accumulatedFrameCount);
        glUniform1f(uniformElapsedTime,    elapsedSeconds);
        glUniform3fv(uniformCameraPosition, 1, glm::value_ptr(camera.position));
        glUniform1f(uniformWindowWidth,    static_cast<float>(WINDOW_WIDTH));
        glUniform1f(uniformWindowHeight,   static_cast<float>(WINDOW_HEIGHT));

        // Bind the previous frame's accumulation texture for blending.
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, currentReadBuffer->colorTextureHandle);
        glUniform1i(uniformAccumTexture, 0);

        glBindVertexArray(fullscreenQuadVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        // ── Pass 2: tonemap to screen ─────────────────────────────────────
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(tonemapProgram);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, currentWriteBuffer->colorTextureHandle);
        glUniform1i(uniformTonemapTexture, 0);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);

        // ── Swap ping-pong buffers ────────────────────────────────────────
        std::swap(currentWriteBuffer, currentReadBuffer);
        ++accumulatedFrameCount;

        glfwSwapBuffers(windowHandle);
        glfwPollEvents();
    }

    // ── Cleanup ───────────────────────────────────────────────────────────────
    glDeleteFramebuffers(1, &accumulationBufferA.framebufferHandle);
    glDeleteTextures(1,     &accumulationBufferA.colorTextureHandle);
    glDeleteFramebuffers(1, &accumulationBufferB.framebufferHandle);
    glDeleteTextures(1,     &accumulationBufferB.colorTextureHandle);
    glDeleteProgram(pathTraceProgram);
    glDeleteProgram(tonemapProgram);

    glfwDestroyWindow(windowHandle);
    glfwTerminate();
    return EXIT_SUCCESS;
}
