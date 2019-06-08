#include "vulkanapi.hpp"
#include <iostream>
#include <algorithm>
#include <set>

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

    VkInstanceCreateInfo createinfo = {};
    createinfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createinfo.pApplicationInfo = &appinfo;

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;

    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    createinfo.enabledExtensionCount = glfwExtensionCount;
    createinfo.ppEnabledExtensionNames = glfwExtensions;
    createinfo.enabledLayerCount = 0;

    if (vkCreateInstance(&createinfo, 0, &instance) != VK_SUCCESS)
        exit(-1);
    
}

void VulkanAPI::InitDevice() {
    uint32_t count = 0;
    vkEnumeratePhysicalDevices(instance, &count, nullptr);
    std::cout << "available physical devices: " << count << std::endl;
    std::vector<VkPhysicalDevice> devices(count);
    vkEnumeratePhysicalDevices(instance, &count, devices.data());
    physDevice = devices[0];

    vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &count, nullptr);
    std::vector<VkQueueFamilyProperties> props(count);
    vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &count, props.data());
    uint32_t prop;
    for (uint32_t a = 0; a < count; a++) {
        VkBool32 pres;
        vkGetPhysicalDeviceSurfaceSupportKHR(physDevice, a, surface, &pres);
        if (pres && (props[a].queueFlags & VK_QUEUE_GRAPHICS_BIT) > 0) {
            prop = a;
            break;
        }
    }

    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = prop;
    queueCreateInfo.queueCount = 1;
    float queuePriority = 1.0f;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkPhysicalDeviceFeatures deviceFeatures = {};

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pEnabledFeatures = &deviceFeatures;

    if(vkCreateDevice(physDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
        exit(-3);
    }

    vkGetDeviceQueue(device, prop, 0, &graphicsQueue);
    vkGetDeviceQueue(device, prop, 0, &presentQueue);
}

void VulkanAPI::CreateSurface() {
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        exit(-2);
    }
}

void VulkanAPI::DestroySurface() {
    vkDestroySurfaceKHR(instance, surface, nullptr);
}

void VulkanAPI::Exit() {
    vkDestroyInstance(instance, nullptr);
}