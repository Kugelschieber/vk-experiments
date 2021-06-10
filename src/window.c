#define GLFW_INCLUDE_VULKAN

#include "window.h"
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include "log.h"

GLFWwindow* vkeCreateWindow(const char* title, int width, int height) {
    int err = glfwInit();

    if(err != GLFW_TRUE) {
        vkeLogError("error initializing glfw");
        return NULL;
    }

    if(glfwVulkanSupported() != GLFW_TRUE) {
        vkeLogError("vulkan not supported");
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

int vkeCreateWindowSurface(VkInstance instance, GLFWwindow* window, VkSurfaceKHR* surface) {
    if(glfwCreateWindowSurface(instance, window, NULL, surface) != VK_SUCCESS) {
        vkeLogError("error creating surface");
        return -1;
    }

    return 0;
}
