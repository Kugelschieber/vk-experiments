#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include "log.h"
#include "window.h"

typedef struct {
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
} VKEContext;

typedef struct {
    const char* title;
    int validationLayerCount;
    const char** validationLayers;
} VKEConfig;

bool vkeCheckValidationLayerSupport(const char** validationLayers, int n) {
    if(n == 0) {
        return true;
    }

    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, NULL);
    VkLayerProperties availableLayers[layerCount];
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);

    for(int i = 0; i < n; i++) {
        bool layerFound = false;

        for(int j = 0; j < layerCount; j++) {
            if(strcmp(validationLayers[i], availableLayers[j].layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if(!layerFound) {
            // log supported validation layers
            for(int j = 0; j < layerCount; j++) {
                vkeLogDebug(availableLayers[j].layerName);
            }

            return false;
        }
    }

    return true;
}

bool vkeIsSuitableDevice(VkPhysicalDevice device) {
    // TODO
    return true;
}

int vkeSelectPhysicalDevice(VKEContext* ctx) {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(ctx->instance, &deviceCount, NULL);

    if(deviceCount == 0) {
        vkeLogError("no physical device available");
        return -1;
    }

    VkPhysicalDevice devices[deviceCount];
    vkEnumeratePhysicalDevices(ctx->instance, &deviceCount, devices);
    VkPhysicalDevice device = VK_NULL_HANDLE;

    for(int i = 0; i < deviceCount; i++) {
        if(vkeIsSuitableDevice(devices[i])) {
            device = devices[i];
            break;
        }
    }

    if(device == VK_NULL_HANDLE) {
        vkeLogError("no suitable physical device found");
        return -2;
    }

    return 0;
}

int vkeInit(VKEContext* ctx, VKEConfig* config) {
    if(!vkeCheckValidationLayerSupport(config->validationLayers, config->validationLayerCount)) {
        vkeLogError("validation layer not supported");
        return -1;
    }

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = config->title,
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_0
    };
    VkInstanceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .enabledExtensionCount = glfwExtensionCount,
        .ppEnabledExtensionNames = glfwExtensions,
        .enabledLayerCount = 0,
        .pApplicationInfo = &appInfo
    };

    if(config->validationLayerCount > 0) {
        createInfo.enabledLayerCount = config->validationLayerCount;
        createInfo.ppEnabledLayerNames = config->validationLayers;
    }
    
    if(vkCreateInstance(&createInfo, NULL, &ctx->instance) != VK_SUCCESS) {
        vkeLogError("error creating vulkan instance");
        return -2;
    }

    if(vkeSelectPhysicalDevice(ctx) != 0) {
        return -3;
    }

    return 0;
}

void vkeDestroy(VKEContext* ctx) {
    vkDestroyInstance(ctx->instance, NULL);
}

void loop(GLFWwindow* window) {
    while(!glfwWindowShouldClose(window)) {
        // ...
        glfwPollEvents();
    }
}

int main(int argc, const char *argv[]) {
    vkeSetLogLevel(0);
    const char* title = "Test";
    GLFWwindow* window = vkeCreateWindow(title, 800, 600);

    if(window == NULL) {
        return -1;
    }

    VKEContext ctx;
    const char* validationLayers[] = {
        "VK_LAYER_KHRONOS_validation"
    };
    VKEConfig config = {
        .title = title,
        .validationLayerCount = 1,
        .validationLayers = validationLayers
    };

    if(vkeInit(&ctx, &config) != 0) {
        return -1;
    }

    loop(window);
    vkeDestroyWindow(window);
    vkeDestroy(&ctx);
    return 0;
}
