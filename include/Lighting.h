#pragma once
#include "Shader.h"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <string>
#include <iostream>

class PointLight {
	private:
	public:
        glm::vec3 lightPos;     // Position of light source
        glm::vec3 pointIntensity; // Point light intensity

        float constantAttenuation;
        float linearAttenuation;
        float quadraticAttenuation;
        int id;
		PointLight(int id, glm::vec3 initLightPos, glm::vec3 pointIntensity, float constantAttenuation, float linearAttenuation, float quadraticAttenuation);
		~PointLight();
        void setPosition(glm::vec3 newPos);
        void setIntensity(glm::vec3 newInten);
};

class GlobalLighting {
    private:
        glm::vec3 ambientLight; // Ambient light intensity
        std::vector<PointLight> pointLights;
    public:
        GlobalLighting(glm::vec3 ambientLight);
        void addPointLight(PointLight& newPointLight);
        void updatePointLightPosition(int id, glm::vec3 newPosition);
        void updatePointLightIntensity(int id, glm::vec3 newIntensity);
        void sendToShader(Shader* shader, glm::vec3 viewPos);
};

class ModelLighting {
    private:
        glm::vec3 ambientCoefficient;   // Ambient-reflection coefficient
        glm::vec3 diffuseCoefficient;   // Diffuse-reflection coefficient
        glm::vec3 specularCoefficient;  // Specular-reflection coefficient
        glm::vec3 specularMetalColour;  // Specular-reflection colour component for metals
        float materialShininess;    // Material Shininess coefficient
    public:
		ModelLighting(
            glm::vec3 ambientCoefficient, 
            glm::vec3 diffuseCoefficient, 
            glm::vec3 specularCoefficient,
            float materialShininess,
            glm::vec3 specularMetalColour = glm::vec3(1.0f)
        );
		~ModelLighting();
        void sendToShader(Shader* shader);
};