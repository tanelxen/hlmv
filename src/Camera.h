//
//  Camera.hpp
//  hlmv
//
//  Created by Fedor Artemenkov on 17.11.24.
//

#pragma once

#include <glm/glm.hpp>

struct GLFWwindow;

class Camera
{
public:
    Camera(GLFWwindow* window);

    void updateViewport(float width, float height);
    void update(float dt);

    glm::mat4 view;
    glm::mat4 projection;

private:
    glm::vec3 position = {0, 48, -96};
    
    float pitch = -0.05;
    float yaw = 1.57;
    
    float moveSpeed = 96;
    float mouseSense = 0.3;

    glm::vec3 velocity = {0, 0, 0};

    double prevMouseX = 0;
    double prevMouseY = 0;
    bool isFirstFrame = true;

    glm::vec3 forward = {0, 0, 0};
    glm::vec3 right = {0, 0, 0};
    glm::vec3 up = {0, 0, 0};

    GLFWwindow* window;
};

