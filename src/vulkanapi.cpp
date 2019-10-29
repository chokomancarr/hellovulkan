#include "vulkanapi.hpp"
#include <iostream>
#include <algorithm>
#include <set>
#include <vector>

VkResult result;

#define VKDO(cmd) if ((result = cmd) != VK_SUCCESS) {\
    std::cerr << #cmd << " failed with error code " << (int)result << std::endl;\
    abort();\
}
#define FNCOK std::cout << __func__ << "() ok" << std::endl;

GLFWwindow* VulkanAPI::window;

VkInstance instance;
VkPhysicalDevice physDevice;
VkDevice device;
VkQueue graphicsQueue;
VkQueue presentQueue;
VkSurfaceKHR surface;

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

void VulkanAPI::CheckExtSupport() {
    uint32_t propc;
    vkEnumerateDeviceExtensionProperties(physDevice, 0, &propc, 0);
    std::vector<VkExtensionProperties> props(propc);
    vkEnumerateDeviceExtensionProperties(physDevice, 0, &propc, props.data());
}

void VulkanAPI::Init() {
    VkApplicationInfo appinfo = {};
    appinfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appinfo.pApplicationName = "Hello Vulkan";
    appinfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appinfo.pEngineName = "No Engine";
    appinfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appinfo.apiVersion = VK_API_VERSION_1_0;

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    if (!glfwExtensionCount) abort();

    VkInstanceCreateInfo createinfo = {};
    createinfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createinfo.pApplicationInfo = &appinfo;
    createinfo.enabledExtensionCount = glfwExtensionCount;
    createinfo.ppEnabledExtensionNames = glfwExtensions;
    createinfo.enabledLayerCount = 0;

    VKDO(vkCreateInstance(&createinfo, 0, &instance));
    
    FNCOK
}

void VulkanAPI::InitDevice() {
    uint32_t count = 0;
    VKDO(vkEnumeratePhysicalDevices(instance, &count, nullptr));
    std::cout << "available physical devices: " << count << std::endl;
    if (!count) exit(0);
    std::vector<VkPhysicalDevice> devices(count);
    VKDO(vkEnumeratePhysicalDevices(instance, &count, devices.data()));
    
    physDevice = devices[0];

    vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &count, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(count);
    vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &count, queueFamilies.data());
    uint32_t graphicsFamily = 0;
    for (uint32_t a = 0; a < count; a++) {
        VkBool32 pres;
        VKDO(vkGetPhysicalDeviceSurfaceSupportKHR(physDevice, a, surface, &pres));
        if (pres && (queueFamilies[a].queueFlags & VK_QUEUE_GRAPHICS_BIT) > 0) {
            graphicsFamily = a;
            break;
        }
    }

    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = graphicsFamily;
    queueCreateInfo.queueCount = 1;
    float queuePriority = 1.0f;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkPhysicalDeviceFeatures deviceFeatures = {};

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pEnabledFeatures = &deviceFeatures;

    VKDO(vkCreateDevice(physDevice, &createInfo, nullptr, &device));

    vkGetDeviceQueue(device, graphicsFamily, 0, &graphicsQueue);
    presentQueue = graphicsQueue;


    FNCOK
}

void VulkanAPI::CreateSurface() {
    VKDO(glfwCreateWindowSurface(instance, window, nullptr, &surface));

    FNCOK
}

void VulkanAPI::DestroySurface() {
    vkDestroySurfaceKHR(instance, surface, nullptr);
}

void VulkanAPI::Exit() {
    vkDestroyDevice(device, nullptr);

    vkDestroyInstance(instance, nullptr);
}