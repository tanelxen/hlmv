
// Default Libraries
#include <stdio.h>

// External Libraries
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "GoldSrcModel.h"
#include "Renderer.h"

static void error_callback(int e, const char *d) { printf("Error %d: %s\n", e, d); }

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

ImVec4 clear_color = ImColor(114, 144, 154);

void imgui_init(GLFWwindow* window);

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
    glfwWindowHint(GLFW_SAMPLES, 8);

    window = glfwCreateWindow(window_width, window_height, "Demo", nullptr, nullptr);

    glfwSetKeyCallback(window, key_callback);

    glfwMakeContextCurrent(window);

    gladLoadGL();

    const unsigned char* version = glGetString(GL_VERSION);
    printf("version: %s\n", version);
    
    const unsigned char* device = glGetString(GL_RENDERER);
    printf("device: %s\n", device);
    
    imgui_init(window);
    
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
        
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        renderer.imgui_draw();
        
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		// Swap front and back buffers
		glfwSwapBuffers(window);

		// Poll for and process events
		glfwPollEvents();
	}
    
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();

    glfwDestroyWindow(window);
	glfwTerminate();
    
	return 0;
}

void imgui_init(GLFWwindow* window)
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 150");

//    setImGuiStyle();
}
