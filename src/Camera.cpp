//
//  Camera.cpp
//  hlmv
//
//  Created by Fedor Artemenkov on 17.11.24.
//

#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include "Camera.h"

Camera::Camera(GLFWwindow* window) : window(window)
{

}

void Camera::update(float dt)
{
    if (isFirstFrame)
    {
        glfwGetCursorPos(window, &prevMouseX, &prevMouseY);
        isFirstFrame = false;
        return;
    }

    double curMouseX = 0;
    double curMouseY = 0;
    glfwGetCursorPos(window, &curMouseX, &curMouseY);

    double dx = curMouseX - prevMouseX;
    double dy = curMouseY - prevMouseY;

    prevMouseX = curMouseX;
    prevMouseY = curMouseY;

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) > GLFW_RELEASE)
    {
        yaw += dx * mouseSense * dt;
        pitch -= dy * mouseSense * dt;

        if (pitch > 1.5) pitch = 1.5;
        else if (pitch < -1.5) pitch = -1.5;

        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
    else
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

    forward.x = cos(yaw) * cos(pitch);
    forward.y = sin(pitch);
    forward.z = sin(yaw) * cos(pitch);
    forward = glm::normalize(forward);

    glm::vec3 unit_up = {0, 1, 0};

    right = glm::cross(forward, unit_up);
    right = glm::normalize(right);

    up = glm::cross(right, forward);
    up = glm::normalize(up);

    velocity = {0, 0, 0};

    if (glfwGetKey(window, GLFW_KEY_W) > GLFW_RELEASE)
    {
        velocity += forward * (moveSpeed * dt);
    }

    if (glfwGetKey(window, GLFW_KEY_S) > GLFW_RELEASE)
    {
        velocity -= forward * (moveSpeed * dt);
    }

    if (glfwGetKey(window, GLFW_KEY_D) > GLFW_RELEASE)
    {
        velocity += right * (moveSpeed * dt);
    }

    if (glfwGetKey(window, GLFW_KEY_A) > GLFW_RELEASE)
    {
        velocity -= right * (moveSpeed * dt);
    }

    position += velocity;
    
    view = glm::lookAt(position, position + forward, up);
}

void Camera::updateViewport(float width, float height)
{
    float ratio = width / height;
    float fov = glm::radians(65.0f);
    projection = glm::perspective(fov, ratio, 0.1f, 4096.0f);
}
