#ifndef VKE_WINDOW_H
#define VKE_WINDOW_H

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

// Creates a new window for given title and dimensions.
// Returns NULL in case of an error.
GLFWwindow* vkeCreateWindow(const char* title, int width, int height);

// Destroys given window.
void vkeDestroyWindow(GLFWwindow* window);

// Create a Vulkan surface for given window.
int vkeCreateWindowSurface(VkInstance instance, GLFWwindow* window, VkSurfaceKHR* surface);

#endif
