//
//  Renderer.cpp
//  hlmv
//
//  Created by Fedor Artemenkov on 11.11.24.
//

#include <thread>

#include "Renderer.h"
#include "GoldSrcModel.h"
#include "Camera.h"
#include "MainQueue.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "../deps/stb_image.h"

#include "../deps/tinyfiledialogs.h"

//#define STB_IMAGE_WRITE_IMPLEMENTATION
//#include "../deps/stb_image_write.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <imgui.h>

Renderer::Renderer()
{
    uploadShader();
}

Renderer::~Renderer()
{
    glDeleteProgram(program);
}

void Renderer::setModel(const Model& model)
{
    m_pmodel = std::make_unique<RenderableModel>();
    m_pmodel->init(model);
    
    sequenceNames.resize(model.sequences.size());

    std::transform(model.sequences.begin(), model.sequences.end(), sequenceNames.begin(), [](const Sequence& seq) {
        return seq.name;
    });
    
//    size_t lastSlashPos = model.name.rfind('/');
//    isPlayerView = lastSlashPos != std::string::npos && model.name.substr(lastSlashPos + 1).starts_with("v_");
}

void Renderer::update(float dt)
{
    if (m_pmodel) {
        m_pmodel->update(dt);
    }
    
    MainQueue::instance().poll();
}

void Renderer::draw(const Camera& camera)
{
    glUseProgram(program);
    
    if (isPlayerView)
    {
        glm::mat4 quakeToGL = {
            {  0,  0, -1,  0 },
            { -1,  0,  0,  0 },
            {  0,  1,  0,  0 },
            {  0,  0,  0,  1 }
        };
        
        quakeToGL[3] = glm::vec4(weaponOffset, 1);
        
        glm::mat4 mvp = camera.projection * quakeToGL;
        glUniformMatrix4fv(u_MVP_loc, 1, GL_FALSE, (const float*) &mvp);
    }
    else
    {
        glm::mat4 quakeToGL = {
            {  0,  0, -1,  0 },
            { -1,  0,  0,  0 },
            {  0,  1,  0,  0 },
            {  0,  0,  0,  1 }
        };
        
        glm::mat4 mvp = camera.projection * camera.view * quakeToGL;
        glUniformMatrix4fv(u_MVP_loc, 1, GL_FALSE, (const float*) &mvp);
    }
    
    if (m_pmodel) {
        glUniformMatrix4fv(u_boneTransforms_loc, (GLsizei)(m_pmodel->transforms.size()), GL_FALSE, &(m_pmodel->transforms[0][0][0]));
        m_pmodel->draw();
    }
}

unsigned int compile_shader(unsigned int type, const char* source);

void Renderer::uploadShader()
{
    const char* vert = R"(
        #version 410 core
        layout (location = 0) in vec4 position;
        layout (location = 1) in vec3 normal;
        layout (location = 2) in vec2 texCoord;
        layout (location = 3) in uint boneIndex;
    
        uniform mat4 uBoneTransforms[128];
        uniform mat4 uMVP;
        
        out vec2 uv;
        out vec3 transformedNormal;
        out vec4 transformedPosition;

        void main()
        {
            transformedPosition = uBoneTransforms[boneIndex] * position;
            transformedNormal = normalize(mat3(uBoneTransforms[boneIndex]) * normal);
            gl_Position = uMVP * transformedPosition;
            uv = texCoord;
        }
    )";
    
    const char* frag = R"(
        #version 410 core
        in vec2 uv;
        in vec3 transformedNormal;
        in vec4 transformedPosition;

        //Texture samplers
        uniform sampler2D s_texture;

        //final color
        out vec4 FragColor;

        void main()
        {
            vec3 lightPos = vec3(128, 128, 128);
            vec3 lightDir = normalize(lightPos - transformedPosition.xyz);
            float nDotL = dot(transformedNormal, lightDir);
            float shade = max(nDotL, 0.0);
    
            float ambient = 0.2;
            shade = min(shade + ambient, 1.0);
    
            FragColor = texture(s_texture, uv) * shade;
        }
    )";
    
    program = glCreateProgram();

    unsigned int vs = compile_shader(GL_VERTEX_SHADER, vert);
    unsigned int fs = compile_shader(GL_FRAGMENT_SHADER, frag);

    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glValidateProgram(program);

    glDeleteShader(vs);
    glDeleteShader(fs);
    
//    glUniform1i(glGetUniformLocation(program, "s_texture"), 0);
    
    glUseProgram(program);
    
    u_MVP_loc = glGetUniformLocation(program, "uMVP");

    if (u_MVP_loc == -1)
    {
        printf("Shader have no uniform %s\n", "uMVP");
    }
    
    u_boneTransforms_loc = glGetUniformLocation(program, "uBoneTransforms");

    if (u_boneTransforms_loc == -1)
    {
        printf("Shader have no uniform %s\n", "uBoneTransforms");
    }
}

void Renderer::imgui_draw()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Open", "Ctrl+O"))
            {
                openFile([this](std::string filename) {

                    Model mdl;
                    mdl.loadFromFile(filename);
                    
                    setModel(mdl);
                    
                }, "*.mdl");
            }

            if (ImGui::MenuItem("Exit")) {
                exit(1);
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
    
    if (m_pmodel == nullptr) return;
    if (m_pmodel->getSeqIndex() >= sequenceNames.size()) return;
    
    ImGui::SetNextWindowSizeConstraints(ImVec2(250, 250), ImVec2(FLT_MAX, FLT_MAX));
    
    if (ImGui::Begin("Model Info###model"))
    {
        ImGui::Text(("Name: " + m_pmodel->name).c_str());
        
        ImGuiStyle& style = ImGui::GetStyle();
        float w = ImGui::CalcItemWidth();
        float spacing = style.ItemInnerSpacing.x;
        float button_sz = ImGui::GetFrameHeight();
        
        ImGui::Text("Sequence");
        ImGui::SameLine(0, 10);
        
        ImGui::PushItemWidth(w - spacing * 2.0f - button_sz * 2.0f);
        
        if (ImGui::BeginCombo("##sequence combo", sequenceNames[m_pmodel->getSeqIndex()].c_str(), ImGuiComboFlags_None))
        {
            for (int i = 0; i < sequenceNames.size(); ++i)
            {
                bool is_selected = (m_pmodel->getSeqIndex() == i);
                
                if (ImGui::Selectable(sequenceNames[i].c_str(), is_selected))
                {
                    m_pmodel->setSeqIndex(i);
                }
                
                if (is_selected) ImGui::SetItemDefaultFocus();
            }
            
            ImGui::EndCombo();
        }
        
        ImGui::PopItemWidth();
        
        ImGui::Checkbox("Player View", &isPlayerView);
        ImGui::InputFloat3("Weapon offset", (float*)&weaponOffset);
        
        ImGui::End();
    }
}

void Renderer::openFile(std::function<void (std::string)> callback, const char* filter)
{
    std::thread([callback, filter]() {
        
        const char* filename = tinyfd_openFileDialog(nullptr, nullptr, 1, &filter, nullptr, 0);
        
        if (filename != nullptr)
        {
            MainQueue::instance().async([callback, filename] () {
                callback(filename);
            });
        }
        
    }).detach();
}


unsigned int compile_shader(unsigned int type, const char* source)
{
    unsigned int id = glCreateShader(type);

    glShaderSource(id, 1, &source, nullptr);
    glCompileShader(id);

    int result;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);

    if (result == GL_FALSE)
    {
        int length;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);

        char message[1024];
        glGetShaderInfoLog(id, length, &length, message);

        printf("Failed to compile %s shader:\n", (type == GL_VERTEX_SHADER) ? "vertex" : "fragment");
        printf("%s\n", message);
        return 0;
    }

    return id;
}
