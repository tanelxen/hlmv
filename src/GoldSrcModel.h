//
//  GoldSrcModel.hpp
//  hlmv
//
//  Created by Fedor Artemenkov on 09.11.24.
//

#pragma once

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

struct MeshVertex
{
    glm::vec3 position; // позиция вершины до применения матрицы
    glm::vec2 texCoord;
    uint8_t boneIndex;  // индекс матрицы в массиве sequences.frames.bonetransforms
};

struct Mesh
{
    std::vector<MeshVertex> vertexBuffer;
    std::vector<unsigned int> indexBuffer;
    int textureIndex;
};

struct Texture
{
    std::string name;
    std::vector<unsigned char> data;
    int width;
    int height;
};

struct Frame
{
    std::vector<glm::quat> rotationPerBone;
    std::vector<glm::vec3> positionPerBone;
};

struct Sequence
{
    std::string name;
    std::vector<Frame> frames;
    float fps;
    float groundSpeed;
};

#include "studio.h"

struct Model
{
    std::string name;
    std::vector<Mesh> meshes;
    std::vector<Texture> textures;
    std::vector<Sequence> sequences;
    std::vector<int> bones;
    
    void loadFromFile(const std::string& filename);
    
private:
    void readTextures();
    void readBodyparts();
    void readSequence();
    
    byte* m_pin;
    studiohdr_t* m_pheader;
};
