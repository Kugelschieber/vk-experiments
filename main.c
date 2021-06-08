#include <stdio.h>
#include <GLFW/glfw3.h>

GLFWwindow* createWindow(const char* title, int width, int height) {
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

void destroyWindow(GLFWwindow* window) {
    glfwDestroyWindow(window);
    glfwTerminate();
}

void loop(GLFWwindow* window) {
    while(!glfwWindowShouldClose(window)) {
        // ...
        glfwPollEvents();
    }
}

int main(int argc, const char *argv[]) {
    GLFWwindow* window = createWindow("Test", 800, 600);

    if(window == NULL) {
        return -1;
    }

    loop(window);
    destroyWindow(window);
    return 0;
}
