//
//  Renderer.hpp
//  hlmv
//
//  Created by Fedor Artemenkov on 11.11.24.
//

#pragma once

#include <vector>
#include <glm/glm.hpp>

#include "GoldSrcModel.h"

struct RenderableMesh
{
    unsigned int vbo;
    unsigned int ibo;
    unsigned int vao;
    unsigned int tex;
    
    int indicesCount;
};

struct GLFWwindow;

struct Renderer
{
    void init();
    
    void init(const Model& model);
    void update(GLFWwindow* window);
    void draw(float dt);
    
private:
    void uploadShader();
    void uploadTextures(const std::vector<Texture>& textures);
    void uploadMeshes(const std::vector<Mesh>& meshes);
    
    void updatePose();
    
    std::vector<Sequence> sequences;
    std::vector<int> bones;
    
    std::vector<RenderableMesh> meshes;
    std::vector<unsigned int> textures;
    
    unsigned int program;
    unsigned int u_MVP_loc;
    unsigned int u_boneTransforms_loc;
    
    float cur_frame = 0;
    float cur_frame_time = 0;
    float cur_anim_duration = 0;
    
    int cur_seq_index = 0;
    
    std::vector<glm::mat4> transforms;
    
    glm::mat4 mvp;
};
