#pragma once
#include "Lighting.h"

class Model {
private:
    GLuint vao = 0;
    GLuint vbo = 0;
    int vertexCount = 0;
    glm::mat4 modelMatrix;
    
    // Helper to upload 12-float stride data (Pos4, Norm4, Col4)
    void uploadToGPU(const std::vector<float>& data);
    
public:
    int id;
    std::string modelName;
    ModelLighting* lighting;
    
    Model(int id, ModelLighting* lighting);
    ~Model();
    Model(const Model&) = delete;
    Model& operator=(const Model&) = delete;

    // OPTION A: Load from File (Uses TinyObjLoader)
    void init(const std::string& filepath);

    // OPTION B: Load from Hardcoded Array
    void init(float* data, int count, const std::string& modelName);

    // STUDENT TASK: Apply transforms
    void update(glm::vec3 position, glm::vec3 rotation, float scale);
	
	void update(glm::mat4 modelMat);

	void draw(Shader* shader);
};
