//
//  Renderer.cpp
//  hlmv
//
//  Created by Fedor Artemenkov on 11.11.24.
//

#include <thread>

#include "Renderer.h"
#include "GoldSrcModel.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "../deps/stb_image.h"

#include "../deps/tinyfiledialogs.h"

//#define STB_IMAGE_WRITE_IMPLEMENTATION
//#include "../deps/stb_image_write.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#define GLM_ENABLE_EXPERIMENTAL 1
#include <glm/gtx/quaternion.hpp>

#include <imgui.h>

void Renderer::init(const Model& model)
{
    this->sequences = model.sequences;
    this->bones = model.bones;
    
    transforms.resize(bones.size());
    
    uploadTextures(model.textures);
    uploadMeshes(model.meshes);
    uploadShader();
    
    sequenceNames.resize(sequences.size());

    std::transform(sequences.begin(), sequences.end(), sequenceNames.begin(), [](Sequence& seq) {
        return seq.name;
    });
    
    cur_frame = 0;
    cur_frame_time = 0;
    cur_anim_duration = 0;
    cur_seq_index = 0;
}

void Renderer::update(GLFWwindow* window)
{
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    
    float ratio = (float)width / height;
    float fov = glm::radians(38.0f);
    glm::mat4x4 projection = glm::perspective(fov, ratio, 0.1f, 4096.0f);
    
    glm::vec3 position = {0, 48, 128};
    glm::vec3 center = {0, 36, 0};
    glm::vec3 up = {0, 1, 0};
    glm::mat4x4 view = glm::lookAt(position, center, up);
    
    // GoldSrc/Quake coordinate system -> OpenGL
    glm::mat4 model = {
        { 0, 0,  1, 0 },
        { 1, 0,  0, 0 },
        { 0, 1,  0, 0 },
        { 0, 0,  0, 1 }
    };
    
    this->mvp = projection * view * model;
    
    if (hasFile)
    {
        hasFile = false;
        
        Model mdl;
        mdl.loadFromFile(filename);
        
        init(mdl);
    }
}

void Renderer::draw(float dt)
{
    if (cur_seq_index >= sequences.size()) return;
    
    Sequence& seq = sequences[cur_seq_index];
    
    cur_anim_duration = (float)seq.frames.size() / seq.fps;
    
    updatePose();
    
    cur_frame_time += dt;
    
    if (cur_frame_time >= cur_anim_duration)
    {
        cur_frame_time = 0;
    }
    
    cur_frame = (float)seq.frames.size() * (cur_frame_time / cur_anim_duration);
    
    glUseProgram(program);
    
    glUniformMatrix4fv(u_MVP_loc, 1, GL_FALSE, (const float*) &mvp);
    glUniformMatrix4fv(u_boneTransforms_loc, (GLsizei)transforms.size(), GL_FALSE, &transforms[0][0][0]);
    
    for (auto& mesh : meshes)
    {
        unsigned int texId = textures[mesh.tex];
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texId);
        
        glBindVertexArray(mesh.vao);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ibo);
        
        glDrawElements(GL_TRIANGLES, mesh.indicesCount, GL_UNSIGNED_INT, 0);
    }
}

void Renderer::uploadTextures(const std::vector<Texture>& textures)
{
    this->textures.resize(textures.size());
    
    for (int i = 0; i < textures.size(); ++i)
    {
        const Texture& item = textures[i];
        
        unsigned int id;
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, item.width, item.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, item.data.data());
        glGenerateMipmap(GL_TEXTURE_2D);
        
        this->textures[i] = id;
        
//        stbi_write_png(item.name.c_str(), item.width, item.height, 4, item.data.data(), item.width * 4);
    }
}

#define VERT_POSITION_LOC 0
#define VERT_DIFFUSE_TEX_COORD_LOC 1
#define VERT_BONE_INDEX_LOC 2

void Renderer::uploadMeshes(const std::vector<Mesh>& meshes)
{
    this->meshes.resize(meshes.size());
    
    for (int i = 0; i < meshes.size(); ++i)
    {
        const Mesh& item = meshes[i];
        
        unsigned int vao;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        
        unsigned int ibo;
        glGenBuffers(1, &ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int) * item.indexBuffer.size(), item.indexBuffer.data(), GL_STATIC_DRAW);

        unsigned int vbo;
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(MeshVertex) * item.vertexBuffer.size(), item.vertexBuffer.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(VERT_POSITION_LOC);
        glVertexAttribPointer(VERT_POSITION_LOC, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)offsetof(MeshVertex, position));

        glEnableVertexAttribArray(VERT_DIFFUSE_TEX_COORD_LOC);
        glVertexAttribPointer(VERT_DIFFUSE_TEX_COORD_LOC, 2, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)offsetof(MeshVertex, texCoord));
        
        glEnableVertexAttribArray(VERT_BONE_INDEX_LOC);
        glVertexAttribIPointer(VERT_BONE_INDEX_LOC, 1, GL_INT, sizeof(MeshVertex), (void*)offsetof(MeshVertex, boneIndex));

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        
        this->meshes[i].indicesCount = item.indexBuffer.size();
        this->meshes[i].tex = item.textureIndex;
        this->meshes[i].vao = vao;
        this->meshes[i].ibo = ibo;
        this->meshes[i].vbo = vbo;
    }
}

unsigned int compile_shader(unsigned int type, const char* source);

void Renderer::uploadShader()
{
    const char* vert = R"(
        #version 410 core
        layout (location = 0) in vec4 position;
        layout (location = 1) in vec2 texCoord;
        layout (location = 2) in uint boneIndex;
    
        uniform mat4 uBoneTransforms[128];
        uniform mat4 uMVP;
        
        out vec2 uv;

        void main()
        {
            gl_Position = uMVP * uBoneTransforms[boneIndex] * position;
            uv = texCoord;
        }
    )";
    
    const char* frag = R"(
        #version 410 core
        in vec2 uv;

        //Texture samplers
        uniform sampler2D s_texture;

        //final color
        out vec4 FragColor;

        void main()
        {
            vec4 o_texture = texture(s_texture, uv);

            FragColor = o_texture;
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

void Renderer::updatePose()
{
    Sequence& seq = sequences[cur_seq_index];
    
    int currIndex = int(cur_frame);
    int nextIndex = (currIndex + 1) % seq.frames.size();
    
    float factor = cur_frame - floor(cur_frame);
    
    Frame& curr = seq.frames[currIndex];
    Frame& next = seq.frames[nextIndex];
    
    for (int i = 0; i < bones.size(); ++i)
    {
        glm::quat& currRotation = curr.rotationPerBone[i];
        glm::quat& nextRotation = next.rotationPerBone[i];
        
        glm::vec3& currPosition = curr.positionPerBone[i];
        glm::vec3& nextPosition = next.positionPerBone[i];
        
        glm::quat rotation = currRotation * (1.0f - factor) + nextRotation * factor;
        glm::vec3 position = currPosition * (1.0f - factor) + nextPosition * factor;
        
        glm::mat4& transform = transforms[i];
        transform = glm::toMat4(rotation);
        
        transform[3][0] = position[0];
        transform[3][1] = position[1];
        transform[3][2] = position[2];
        
        if (bones[i] != -1)
        {
            transform = transforms[bones[i]] * transform;
        }
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
                    this->filename = filename;
                    this->hasFile = true;
                }, "*.mdl");
            }

            if (ImGui::MenuItem("Exit")) {
                exit(1);
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
    
    if (cur_seq_index >= sequenceNames.size()) return;
    
    ImGuiStyle& style = ImGui::GetStyle();
    float w = ImGui::CalcItemWidth();
    float spacing = style.ItemInnerSpacing.x;
    float button_sz = ImGui::GetFrameHeight();
    
    ImGui::Text("Sequence");
    ImGui::SameLine(0, 10);
    
    ImGui::PushItemWidth(w - spacing * 2.0f - button_sz * 2.0f);
    
    if (ImGui::BeginCombo("##sequence combo", sequenceNames[cur_seq_index].c_str(), ImGuiComboFlags_None))
    {
        for (int i = 0; i < sequenceNames.size(); ++i)
        {
            bool is_selected = (cur_seq_index == i);
            
            if (ImGui::Selectable(sequenceNames[i].c_str(), is_selected))
            {
                cur_seq_index = i;
                cur_frame_time = 0;
                cur_frame = 0;
            }
            
            if (is_selected) ImGui::SetItemDefaultFocus();
        }
        
        ImGui::EndCombo();
    }
    
    ImGui::PopItemWidth();
}

void Renderer::openFile(std::function<void (std::string)> callback, const char* filter)
{
    std::thread f = std::thread([callback, filter]() {
        const char* filename = tinyfd_openFileDialog(nullptr, nullptr, 1, &filter, nullptr, 0);
        callback(std::string(filename));
    });
    f.detach();
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
