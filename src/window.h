#ifndef VKE_WINDOW_H
#define VKE_WINDOW_H

#include <GLFW/glfw3.h>

// Creates a new window for given title and dimensions.
// Returns NULL in case of an error.
GLFWwindow* vkeCreateWindow(const char* title, int width, int height);

// Destroys given window.
void vkeDestroyWindow(GLFWwindow* window);

#endif
