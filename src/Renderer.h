//
//  Renderer.hpp
//  hlmv
//
//  Created by Fedor Artemenkov on 11.11.24.
//

#pragma once

#include <vector>
#include <functional>

#include <glm/glm.hpp>

#include "GoldSrcModel.h"
#include "RenderableModel.h"

struct GLFWwindow;
class Camera;

struct Renderer
{
    Renderer();
    ~Renderer();
    
    void setModel(const Model& model);
    void update(float dt);
    void draw(const Camera& camera);
    void imgui_draw();
    
private:
    void uploadShader();
    
    unsigned int program;
    unsigned int u_MVP_loc;
    unsigned int u_boneTransforms_loc;
    
    std::unique_ptr<RenderableModel> m_pmodel;
    
    //ImGui stuff
    std::vector<std::string> sequenceNames;
    
    bool isPlayerView = false;
    glm::vec3 weaponOffset = {0, 0, 0};
    
    void openFile(std::function<void(std::string)> callback, const char* filter);
};
