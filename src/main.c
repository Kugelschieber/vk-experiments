#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include "util.h"
#include "log.h"
#include "window.h"

typedef struct {
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkSurfaceKHR surface;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkSwapchainKHR swapChain;
} VKEContext;

typedef struct {
    const char* title;
    uint32_t validationLayerCount;
    const char** validationLayers;
    uint32_t deviceExtensionCount;
    const char** deviceExtensions;
} VKEConfig;

typedef struct {
    uint32_t graphicsQueueFamily;
    uint32_t presentQueueFamily;
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
    bool foundPresentQueue = false;

    for(int i = 0; i < queueFamilyCount; i++) {
        if(queueFamilies[i].queueFlags&VK_QUEUE_GRAPHICS_BIT) {
            indices->graphicsQueueFamily = i;
            foundGraphicsQueue = true;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(ctx->physicalDevice, i, ctx->surface, &presentSupport);

        if(presentSupport) {
            indices->presentQueueFamily = i;
            foundPresentQueue = true;
        }
    }

    if(!foundGraphicsQueue) {
        vkeLogError("graphics queue family index not found");
        return -1;
    }

    if(!foundPresentQueue) {
        vkeLogError("present queue family index not found");
        return -1;
    }

    return 0;
}

int vkeCreateLogicalDeviceAndQueues(VKEContext* ctx, VKEConfig* config, VKEQueueFamilyIndices* queueFamilies) {
    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo[] = {
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = queueFamilies->graphicsQueueFamily,
            .queueCount = 1,
            .pQueuePriorities = &queuePriority
        },
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = queueFamilies->presentQueueFamily,
            .queueCount = 1,
            .pQueuePriorities = &queuePriority
        }
    };
    //VkPhysicalDeviceFeatures deviceFeatures; TODO
    VkDeviceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pQueueCreateInfos = queueCreateInfo,
        .queueCreateInfoCount = 2,
        .pEnabledFeatures = VK_NULL_HANDLE,
        .enabledExtensionCount = config->deviceExtensionCount,
        .ppEnabledExtensionNames = config->deviceExtensions
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
    vkGetDeviceQueue(ctx->device, queueFamilies->presentQueueFamily, 0, &ctx->presentQueue);
    return 0;
}

int vkeCreateSwapChain(VKEContext* ctx, VKEQueueFamilyIndices* queueFamilies, GLFWwindow* window) {
    // collect surface capabilities
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx->physicalDevice, ctx->surface, &capabilities);
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->physicalDevice, ctx->surface, &formatCount, NULL);
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(ctx->physicalDevice, ctx->surface, &presentModeCount, NULL);

    if(formatCount == 0) {
        vkeLogError("no color formats available");
        return -1;
    }

    if(presentModeCount == 0) {
        vkeLogError("no present modes available");
        return -2;
    }

    VkSurfaceFormatKHR formats[formatCount];
    vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->physicalDevice, ctx->surface, &formatCount, formats);
    VkPresentModeKHR presentModes[presentModeCount];
    vkGetPhysicalDeviceSurfacePresentModesKHR(ctx->physicalDevice, ctx->surface, &presentModeCount, presentModes);

    // select the color format
    VkSurfaceFormatKHR format = formats[0];

    for(int i = 0; i < formatCount; i++) {
        if(formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            format = formats[i];
            break;
        }
    }

    // select present mode
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;

    for(int i = 0; i < presentModeCount; i++) {
        if(presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            presentMode = presentModes[i];
            break;
        }
    }

    // choose extent
    VkExtent2D extent;

    if(capabilities.currentExtent.width != UINT32_MAX) {
        extent = capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        VkExtent2D actualExtent = {width, height};
        actualExtent.width = max(capabilities.minImageExtent.width, min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = max(capabilities.minImageExtent.height, min(capabilities.maxImageExtent.height, actualExtent.height));
        extent = actualExtent;
    }

    // select the image count
    uint32_t imageCount = capabilities.minImageCount + 1;

    if(capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }

    // create the swap chain
    VkSwapchainCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = ctx->surface,
        .minImageCount = imageCount,
        .imageFormat = format.format,
        .imageColorSpace = format.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .preTransform = capabilities.currentTransform,
        .presentMode = presentMode,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE
    };

    if(queueFamilies->graphicsQueueFamily != queueFamilies->presentQueueFamily) {
        const uint32_t indices[] = {queueFamilies->graphicsQueueFamily, queueFamilies->presentQueueFamily};
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = indices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = NULL;
    }

    if(vkCreateSwapchainKHR(ctx->device, &createInfo, NULL, &ctx->swapChain) != VK_SUCCESS) {
        return -3;
    }

    return 0;
}

int vkeInit(VKEContext* ctx, VKEConfig* config, GLFWwindow* window) {
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

    if(vkeCreateWindowSurface(ctx->instance, window, &ctx->surface) != 0) {
        return -6;
    }

    VKEQueueFamilyIndices queueFamilies;

    if(vkeFindQueueFamilies(ctx, &queueFamilies) != 0) {
        return -4;
    }

    if(vkeCreateLogicalDeviceAndQueues(ctx, config, &queueFamilies) != 0) {
        return -5;
    }

    if(vkeCreateSwapChain(ctx, &queueFamilies, window) != 0) {
        return -6;
    }

    return 0;
}

void vkeDestroy(VKEContext* ctx) {
    vkDestroySwapchainKHR(ctx->device, ctx->swapChain, NULL);
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
    const char* deviceExtensions[] = {
        "VK_KHR_swapchain"
    };
    VKEConfig config = {
        .title = title,
        .validationLayerCount = 1,
        .validationLayers = validationLayers,
        .deviceExtensionCount = 1,
        .deviceExtensions = deviceExtensions
    };

    if(vkeInit(&ctx, &config, window) != 0) {
        return -1;
    }

    loop(window);
    vkeDestroyWindow(window);
    vkeDestroy(&ctx);
    return 0;
}
