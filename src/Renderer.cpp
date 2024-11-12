//
//  Renderer.cpp
//  hlmv
//
//  Created by Fedor Artemenkov on 11.11.24.
//

#include "Renderer.h"
#include "GoldSrcModel.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "../deps/stb_image.h"

//#define STB_IMAGE_WRITE_IMPLEMENTATION
//#include "../deps/stb_image_write.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#define GLM_ENABLE_EXPERIMENTAL 1
#include <glm/gtx/quaternion.hpp>

void Renderer::init(const Model& model)
{
    this->sequences = model.sequences;
    this->bones = model.bones;
    
    transforms.resize(bones.size());
    
    uploadTextures(model.textures);
    uploadMeshes(model.meshes);
    uploadShader();
}

void Renderer::update(GLFWwindow* window)
{
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    
    float ratio = (float)width / height;
    float fov = glm::radians(38.0f);
    glm::mat4x4 projection = glm::perspective(fov, ratio, 0.1f, 4096.0f);
    
    glm::vec3 position = {128, -128, 128};
    glm::vec3 center = {0, 0, 32};
    glm::vec3 up = {0, 0, 1};
    glm::mat4x4 view = glm::lookAt(position, center, up);
    
    this->mvp = projection * view;
}

void Renderer::draw(float dt)
{
    Sequence& seq = sequences[2];
    
    float frameDuration = 1.0f / seq.fps;
    m_timeAccumulator += dt;

    // Если накопленное время больше или равно длительности одного кадра
    while (m_timeAccumulator >= frameDuration)
    {
        m_timeAccumulator -= frameDuration;
        // Переход к следующему кадру, если достигнут конец - возвращаемся к началу
        m_currentFrameIndex = (m_currentFrameIndex + 1) % seq.frames.size();
    }
    
    Frame& frame = seq.frames[m_currentFrameIndex];
    
    for (int i = 0; i < bones.size(); ++i)
    {
        glm::mat4& transform = transforms[i];
        
        transform = glm::toMat4(frame.rotationPerBone[i]);
        
        transform[3][0] = frame.positionPerBone[i][0];
        transform[3][1] = frame.positionPerBone[i][1];
        transform[3][2] = frame.positionPerBone[i][2];
        
        if (bones[i] != -1)
        {
            transform = transforms[bones[i]] * transform;
        }
    }
    
    glUseProgram(program);
    
    glUniformMatrix4fv(u_MVP_loc, 1, GL_FALSE, (const float*) &mvp);
    
    for (auto& mesh : meshes)
    {
        glUniformMatrix4fv(u_boneTransforms_loc, (GLsizei)transforms.size(), GL_FALSE, glm::value_ptr(transforms[0]));
        
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
        glVertexAttribPointer(VERT_BONE_INDEX_LOC, 1, GL_BYTE, GL_FALSE, sizeof(MeshVertex), (void*)offsetof(MeshVertex, boneIndex));

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        
        this->meshes[i].tex = item.textureIndex;
        this->meshes[i].vao = vao;
        this->meshes[i].ibo = ibo;
        this->meshes[i].vbo = vbo;
        this->meshes[i].indicesCount = item.indexBuffer.size();
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
