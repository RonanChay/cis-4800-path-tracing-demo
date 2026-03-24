#include "Model.h"
#include <iostream>

// Define this ONLY here. 
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

Model::Model(int id, ModelLighting* lighting) { 
    modelMatrix = glm::mat4(1.0f); 
    this->lighting = lighting;
    this->id = id;
}

Model::~Model() {
    if (vao != 0) glDeleteVertexArrays(1, &vao);
    if (vbo != 0) glDeleteBuffers(1, &vbo);
}

// --- INSTRUCTOR PROVIDED: DATA UPLOAD ---
void Model::uploadToGPU(const std::vector<float>& data) {
    vertexCount = data.size() / 12; // 12 floats per vertex

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_STATIC_DRAW);

    // Stride = 12 floats (4 Pos, 4 Norm, 4 Col)
    GLsizei stride = 12 * sizeof(float);
    
    // Position
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    // Normal
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (void*)(4 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // Color
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride, (void*)(8 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    glBindVertexArray(0);
}

// --- Load OBJ ---
void Model::init(const std::string& filepath) {
    this->modelName = filepath.substr(filepath.find_last_of("/\\") + 1);

    tinyobj::ObjReaderConfig reader_config;
    tinyobj::ObjReader reader;

    if (!reader.ParseFromFile(filepath, reader_config)) {
        if (!reader.Error().empty()) std::cerr << "TinyObjReader: " << reader.Error();
        return;
    }
	
	// std::cout << "Model "<< filepath << " Loaded" << std::endl;

    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();

    std::vector<float> vertices;
    // Flatten data: Pos(4) + Norm(4) + Col(4)
    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            // POS
            vertices.push_back(attrib.vertices[3 * index.vertex_index + 0]);
            vertices.push_back(attrib.vertices[3 * index.vertex_index + 1]);
            vertices.push_back(attrib.vertices[3 * index.vertex_index + 2]);
            vertices.push_back(1.0f);
            
            // NORM
            if (index.normal_index >= 0) {
                vertices.push_back(attrib.normals[3 * index.normal_index + 0]);
                vertices.push_back(attrib.normals[3 * index.normal_index + 1]);
                vertices.push_back(attrib.normals[3 * index.normal_index + 2]);
                vertices.push_back(0.0f);
            } else { vertices.insert(vertices.end(), {0,0,0,0}); }

            // COL (White)
            vertices.insert(vertices.end(), {1.0f, 1.0f, 1.0f, 1.0f});
        }
    }
    uploadToGPU(vertices);
}

// --- Hardcoded Array ---
void Model::init(float* data, int count, const std::string& modelName) {
    this->modelName = modelName;
    // Convert raw array to vector for the uploader
    std::vector<float> vecData(data, data + count);
    uploadToGPU(vecData);
}

void Model::draw(Shader* shader) {
    if (vao == 0) return;

    shader->Bind();

    // Use your existing Shader class function!
    // Make sure your shader uniform is named "u_model" or change string below
    shader->SetUniform4m("u_model", modelMatrix);
    
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    glBindVertexArray(0);
}

// --- STUDENT TASK: TRANSFORMATIONS ---
void Model::update(glm::vec3 position, glm::vec3 rotation, float scale) {
    modelMatrix = glm::mat4(1.0f); // Identity

    // Student must add: Translate -> Rotate -> Scale
    modelMatrix = glm::translate(modelMatrix, position);
    modelMatrix = glm::rotate(modelMatrix, 1.0f, rotation);
    modelMatrix = glm::scale(modelMatrix, glm::vec3(scale));
}

void Model::update(glm::mat4 modelMat) {
    modelMatrix = modelMat;
}