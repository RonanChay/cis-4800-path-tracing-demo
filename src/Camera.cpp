#include "Camera.h"
#include <iostream>

Camera::Camera(GLFWwindow* window, float sensitivity, float speed) {
    this->window = window;
    this->position = glm::vec3(0.0f, 0.0f, 0.0f); // Optional: Default start pos
    this->forward = glm::vec3(1.0, 0.0, 0.0);   // Start parallel to z-axis
    this->yaw = 0.0f; // Face -Z
    this->pitch = 0.0f;
	
	this->sens = sensitivity;
	this->spd=speed;

    this->lastX = 0.0;
    this->lastY = 0.0;
    this->pitchBoundary = 89.0f;   // Max pitch possible
}

Camera::~Camera(){}

glm::vec3 Camera::getPosition() {
    return this->position;
}

/// @brief Toggle between orthographic and perspective projection
void Camera::toggleProjection() {
    this->useOrtho = !this->useOrtho;
}

/// @brief Update position based on the keyboard input
/// @param forward forward direction
/// @param right right direction
/// @param move_speed movement speed
void Camera::processKeyboardInput(glm::vec3 right, float move_speed) {
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
            this->position += up*move_speed;
        } else {
            this->position += forward*move_speed;
        }
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        this->position -= right*move_speed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
            this->position -= up*move_speed;
        } else {
            this->position -= forward*move_speed;
        }
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        this->position += right*move_speed;
}

/// @brief Update viewing angle based on mouse input
/// @param sensitivity mouse sensitivity
void Camera::processMouseInput(float sensitivity) {
    double currentX, currentY;
    glfwGetCursorPos(window, &currentX, &currentY);
    float deltaX = ((float)(currentX - lastX)) * sensitivity;
    float deltaY = -((float)(currentY - lastY)) * sensitivity;
    this->lastX = currentX;
    this->lastY = currentY;

    this->yaw += deltaX;
    this->pitch += deltaY;
    if (this->pitch > this->pitchBoundary) {
        this->pitch = this->pitchBoundary;
    }
    if (this->pitch < -this->pitchBoundary) {
        this->pitch = -this->pitchBoundary;
    }
}

/// @brief Update based on keyboard and mouse inputs
/// @param deltaTime Amount of time elapsed between frames
void Camera::update(float deltaTime) {
	float speed = this->spd * deltaTime;
	float sensitivity = this->sens * deltaTime;

    // --- TASK 1: MOUSE INPUT (Yaw/Pitch) ---
    this->processMouseInput(sensitivity);

    // --- TASK 2: CALCULATE VECTORS ---
    // 1. Calculate new Forward Vector
    this->forward = glm::normalize(glm::vec3(
        glm::cos(glm::radians(this->yaw)) * glm::cos(glm::radians(this->pitch)),
        glm::sin(glm::radians(this->pitch)),
        glm::sin(glm::radians(this->yaw)) * glm::cos(glm::radians(this->pitch))
    ));
    
    // 2. Calculate Right Vector
    // Hint: Cross product of Forward and World Up (0,1,0) gives vector pointing Right
    glm::vec3 right = glm::normalize(glm::cross(this->forward, glm::vec3(0.0, 1.0, 0.0)));

    // 3. Calculate Up Vector
    // Hint: Cross product of Right and Forward gives the Camera's local Up
    this->up = glm::normalize(glm::cross(right, this->forward));

    // --- TASK 3: MOVEMENT (WASD) ---
    // Move POSITION using the calculated vectors
    this->processKeyboardInput(right, speed);

}

/// @brief Construct and return view matrix
/// @return View matrix
glm::mat4 Camera::getViewMatrix() {
    // LookAt arguments: (Camera Position, Target Position, Up Vector)
    return glm::lookAt(this->position, this->position + this->forward, this->up);
}

/// @brief Construct and return projection matrix
/// @return Projection matrix
glm::mat4 Camera::getProjectionMatrix() {
	int windowWidth, windowHeight;
	glfwGetFramebufferSize(window, &windowWidth, &windowHeight);
	
	if (windowHeight == 0) windowHeight = 1; // prevent div by 0 on minimize
    float aspect = (float)windowWidth / (float)windowHeight;
	
	if (useOrtho) {
        // ORTHOGRAPHIC (The "Box" View)
        float zoom = 1.0f; 
        return glm::ortho(-zoom * aspect, zoom * aspect, -zoom, zoom, 0.1f, 100.0f);
    }
    return glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
}

/// @brief Send view and perspective matrices to shader
/// @param shader 
void Camera::sendToShader(Shader* shader) {
    // 1. Calculate matrices (or use cached ones)
    glm::mat4 view = getViewMatrix();
    glm::mat4 projection = getProjectionMatrix(); 

    shader->Bind();

    // 2. Send to Shader using your helper function
    // We assume standard naming: "u_view" and "u_projection"
    shader->SetUniform4m("u_view", view);
    shader->SetUniform4m("u_projection", projection);
}
