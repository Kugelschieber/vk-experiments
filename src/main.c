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
    VkDevice device;
    VkQueue graphicsQueue;
    VkSurfaceKHR surface;
} VKEContext;

typedef struct {
    const char* title;
    int validationLayerCount;
    const char** validationLayers;
} VKEConfig;

typedef struct {
    uint32_t graphicsQueueFamily;
} VKEQueueFamilyIndices;

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

    ctx->physicalDevice = device;
    return 0;
}

int vkeFindQueueFamilies(VKEContext* ctx, VKEQueueFamilyIndices* indices) {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(ctx->physicalDevice, &queueFamilyCount, NULL);
    VkQueueFamilyProperties queueFamilies[queueFamilyCount];
    vkGetPhysicalDeviceQueueFamilyProperties(ctx->physicalDevice, &queueFamilyCount, queueFamilies);
    bool foundGraphicsQueue = false;

    for(int i = 0; i < queueFamilyCount; i++) {
        if(queueFamilies[i].queueFlags&VK_QUEUE_GRAPHICS_BIT) {
            indices->graphicsQueueFamily = i;
            foundGraphicsQueue = true;
        }
    }

    if(!foundGraphicsQueue) {
        vkeLogError("graphics queue family index not found");
        return -1;
    }

    return 0;
}

int vkeCreateLogicalDeviceAndQueues(VKEContext* ctx, VKEConfig* config, VKEQueueFamilyIndices* queueFamilies) {
    VkDeviceQueueCreateInfo queueCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = queueFamilies->graphicsQueueFamily,
        .queueCount = 1
    };
    float queuePriority = 1.0f;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    //VkPhysicalDeviceFeatures deviceFeatures;
    VkDeviceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pQueueCreateInfos = &queueCreateInfo,
        .queueCreateInfoCount = 1,
        .pEnabledFeatures = VK_NULL_HANDLE,
        .enabledExtensionCount = 0
    };

    if(config->validationLayerCount > 0) {
        createInfo.enabledLayerCount = config->validationLayerCount;
        createInfo.ppEnabledLayerNames = config->validationLayers;
    } else {
        createInfo.enabledLayerCount = 0;
    }

    if(vkCreateDevice(ctx->physicalDevice, &createInfo, NULL, &ctx->device) != VK_SUCCESS) {
        vkeLogError("error creating logical device");
        return -1;
    }

    vkGetDeviceQueue(ctx->device, queueFamilies->graphicsQueueFamily, 0, &ctx->graphicsQueue);
    return 0;
}

int vkeCreateSurface(VKEContext* ctx) {
    VkDisplaySurfaceCreateInfoKHR surfaceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR
        // TODO
    };

    if(vkCreateDisplayPlaneSurfaceKHR(ctx->instance, &surfaceCreateInfo, NULL, &ctx->surface) != VK_SUCCESS) {
        vkeLogError("error creating surface");
        return -1;
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

    VKEQueueFamilyIndices queueFamilies;

    if(vkeFindQueueFamilies(ctx, &queueFamilies) != 0) {
        return -4;
    }

    if(vkeCreateLogicalDeviceAndQueues(ctx, config, &queueFamilies) != 0) {
        return -5;
    }

    // TODO
    /*if(vkeCreateSurface(ctx) != 0) {
        return -6;
    }*/

    return 0;
}

void vkeDestroy(VKEContext* ctx) {
    vkDestroySurfaceKHR(ctx->instance, ctx->surface, NULL);
    vkDestroyDevice(ctx->device, NULL);
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
