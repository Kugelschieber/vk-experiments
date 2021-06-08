#include "window.h"
#include <stdio.h>
#include <GLFW/glfw3.h>

GLFWwindow* vkeCreateWindow(const char* title, int width, int height) {
    int err = glfwInit();

    if(err != GLFW_TRUE) {
        perror("error initializing glfw");
        return NULL;
    }

    if(glfwVulkanSupported() != GLFW_TRUE) {
        perror("vulkan not supported");
        return NULL;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(width, height, title, NULL, NULL);
    glfwSetWindowSizeLimits(window, width, height, width, height);
    return window;
}

void vkeDestroyWindow(GLFWwindow* window) {
    glfwDestroyWindow(window);
    glfwTerminate();
}
