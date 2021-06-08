#include <stdio.h>
#include <GLFW/glfw3.h>
#include "window.h"

void loop(GLFWwindow* window) {
    while(!glfwWindowShouldClose(window)) {
        // ...
        glfwPollEvents();
    }
}

int main(int argc, const char *argv[]) {
    GLFWwindow* window = vkeCreateWindow("Test", 800, 600);

    if(window == NULL) {
        return -1;
    }

    loop(window);
    vkeDestroyWindow(window);
    return 0;
}
