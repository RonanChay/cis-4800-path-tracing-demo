#include "Lighting.h"
#include <format>

PointLight::PointLight(int id, glm::vec3 initLightPos, glm::vec3 pointIntensity, float constantAttenuation, float linearAttenuation, float quadraticAttenuation) {
    this->id = id;
    this->lightPos = initLightPos;
    this->pointIntensity = pointIntensity;
    this->constantAttenuation = constantAttenuation;
    this->linearAttenuation = linearAttenuation;
    this->quadraticAttenuation = quadraticAttenuation;
}
PointLight::~PointLight(){}
void PointLight::setPosition(glm::vec3 newPos) {
    this->lightPos = newPos;
}
void PointLight::setIntensity(glm::vec3 newInten) {
    this->pointIntensity = newInten;
}


GlobalLighting::GlobalLighting(glm::vec3 ambientLight) {
    this->ambientLight = ambientLight;
};
void GlobalLighting::addPointLight(PointLight& newPointLight) {
    if (pointLights.size() >= 10) {
        printf("Dev Error: Too many point lights. Max is 10. Change GlobalLighting::addPointLight() and in shader if need more");
        exit(1);
    }
    pointLights.push_back(newPointLight);
}
void GlobalLighting::updatePointLightPosition(int id, glm::vec3 newPosition) {
    for (auto i = pointLights.begin(); i != pointLights.end(); i++) {
        if (i->id == id) {
            i->setPosition(newPosition);
            return;
        }
    }
    printf(">> Could not find point light with id %d\n", id);
}
void GlobalLighting::updatePointLightIntensity(int id, glm::vec3 newInten) {
    for (auto i = pointLights.begin(); i != pointLights.end(); i++) {
        if (i->id == id) {
            i->setIntensity(newInten);
            return;
        }
    }
    printf(">> Could not find point light with id %d\n", id);
}
void GlobalLighting::sendToShader(Shader* shader, glm::vec3 viewPos) {
    shader->Bind();
    shader->SetUniform3f("u_viewpos", viewPos);
    shader->SetUniform3f("u_ambientIntensity", this->ambientLight);
    shader->SetUniform1i("u_num_pointLights", this->pointLights.size());
    
    for (int i = 0; i < pointLights.size(); i++) {
        std::string uniformArrayIndex("u_pointLights[");
        uniformArrayIndex = uniformArrayIndex + std::to_string(i) + std::string("].");
        shader->SetUniform3f((uniformArrayIndex+std::string("position")).c_str(), pointLights[i].lightPos);
        shader->SetUniform3f((uniformArrayIndex+std::string("point_intensity")).c_str(), pointLights[i].pointIntensity);
        shader->SetUniform1f((uniformArrayIndex+std::string("constant_attenuation")).c_str(), pointLights[i].constantAttenuation);
        shader->SetUniform1f((uniformArrayIndex+std::string("linear_attenuation")).c_str(), pointLights[i].linearAttenuation);
        shader->SetUniform1f((uniformArrayIndex+std::string("quadratic_attenuation")).c_str(), pointLights[i].quadraticAttenuation);
    }
}

ModelLighting::ModelLighting(
    glm::vec3 ambientCoefficient, 
    glm::vec3 diffuseCoefficient, 
    glm::vec3 specularCoefficient, 
    float materialShininess,
    glm::vec3 specularMetalColour
) {
    this->ambientCoefficient = ambientCoefficient;
    this->diffuseCoefficient = diffuseCoefficient;
    this->specularCoefficient = specularCoefficient;
    this->specularMetalColour = specularMetalColour;
    this->materialShininess = materialShininess;
}
ModelLighting::~ModelLighting(){}
void ModelLighting::sendToShader(Shader* shader) {
    shader->Bind();

    shader->SetUniform3f("u_ambientCoeff", this->ambientCoefficient);
    shader->SetUniform3f("u_diffuseCoeff", this->diffuseCoefficient);
    shader->SetUniform3f("u_specularCoeff", this->specularCoefficient);
    shader->SetUniform3f("u_specularMetalColour", this->specularMetalColour);
    shader->SetUniform1f("u_materialShininess", this->materialShininess);
};