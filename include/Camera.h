#pragma once
#include "Shader.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <string>
#include <iostream>

class Camera {
	private:
		GLFWwindow* window; 
		bool useOrtho = false;  // Orthographic or Perspective projection
		glm::vec3 position; // (x,y,z) of camera position in world
		glm::vec3 forward;  // Direction camera is pointing
		glm::vec3 up;       // Normal of forward pointing up for orientation
		float yaw;          // Movement yaw in rad
        float pitch;        // Movement pitch in rad
	
		float spd;          // Movement speed (how fast position changes)
		float sens;         // Camera sensitivity
	
		float lastX, lastY; // Previous mouse coordinates
        float pitchBoundary;    // Maximum/minimum pitch allowed
        
        void processKeyboardInput(glm::vec3 right, float deltaTime);
        void processMouseInput(float sensitivity);
	public:
		Camera(GLFWwindow* window, float sensitivity, float speed);
		~Camera();
        glm::vec3 getPosition();
		void toggleProjection();
		void update(float deltaTime);
		glm::mat4 getViewMatrix();
		glm::mat4 getProjectionMatrix();
		void sendToShader(Shader* shader);
};