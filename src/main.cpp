/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*
****/
// todo:
// - clean up this mess

// updates:
// 1-4-98		fixed initialization
// 23-11-2018	moved from GLUT to GLFW, reimplemented zooming

// Default Libraries
#include <stdio.h>

// External Libraries
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "GoldSrcModel.h"
#include "Renderer.h"

#pragma warning( disable : 4244 ) // conversion from 'double ' to 'float ', possible loss of data
#pragma warning( disable : 4305 ) // truncation from 'const double ' to 'float '

#define APPLICATION_NAME "Half-Life 1 Model Viewer"
#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 800

static void error_callback(int e, const char *d) { printf("Error %d: %s\n", e, d); }

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

int main()
{
    /* Platform */
    static GLFWwindow *window;
    const int window_width = 1440;
    const int window_height = 900;

    /* GLFW */
    glfwSetErrorCallback(error_callback);

    if (!glfwInit()) {
        fprintf(stdout, "[GFLW] failed to init!\n");
        exit(1);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GL_TRUE);
#endif

    glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
//    glfwWindowHint(GLFW_SAMPLES, 8);

    window = glfwCreateWindow(window_width, window_height, "Demo", nullptr, nullptr);

    glfwSetKeyCallback(window, key_callback);

    glfwMakeContextCurrent(window);

    gladLoadGL();

    const unsigned char* version = glGetString(GL_VERSION);
    printf("version: %s\n", version);
    
    const unsigned char* device = glGetString(GL_RENDERER);
    printf("device: %s\n", device);
    
    glfwSwapInterval(1);

    glEnable(GL_DEPTH_TEST);
    glFrontFace(GL_CCW);
    
    Model mdl;
    mdl.loadFromFile("assets/gman.mdl");
    
    Renderer renderer;
    renderer.init(mdl);
    
    double prevTime = 0;
    double deltaTime;

	// Loop until the user closes the window
	while (!glfwWindowShouldClose(window))
	{
        double currTime = glfwGetTime();
        deltaTime = currTime - prevTime;
        prevTime = currTime;
        
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        
        renderer.update(window);
        
        glViewport(0, 0, width, height);
        
        glClearColor(0.1, 0.1, 0.1, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        renderer.draw(deltaTime);

		// Swap front and back buffers
		glfwSwapBuffers(window);

		// Poll for and process events
		glfwPollEvents();
	}

    glfwDestroyWindow(window);
	glfwTerminate();
    
	return 0;
}