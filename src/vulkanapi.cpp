#include "vulkanapi.hpp"
#include <iostream>
#include <algorithm>
#include <set>
#include <vector>
#include <cstdint>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

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
uint32_t graphicsFamily;
VkQueue graphicsQueue;
VkQueue presentQueue;
VkSurfaceKHR surface;
VkSwapchainKHR swapchain;
std::vector<VkImage> swapchainImages;

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

void VulkanAPI::Init() {
    window = glfwCreateWindow(WIDTH, HEIGHT, "Hello Vulkan", 0, 0);

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
    createInfo.enabledExtensionCount = deviceExtensions.size();
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    VKDO(vkCreateDevice(physDevice, &createInfo, nullptr, &device));

    vkGetDeviceQueue(device, graphicsFamily, 0, &graphicsQueue);
    presentQueue = graphicsQueue;

    FNCOK
}

void VulkanAPI::CreateSurface() {
    VKDO(glfwCreateWindowSurface(instance, window, nullptr, &surface));

    FNCOK
}

void VulkanAPI::CreateSwapchain() {
    uint32_t surfaceFormatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physDevice, surface, &surfaceFormatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physDevice, surface, &surfaceFormatCount, surfaceFormats.data());
    
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physDevice, surface, &presentModeCount, nullptr);
    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physDevice, surface, &presentModeCount, presentModes.data());

    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physDevice, surface, &capabilities);

    VkSurfaceFormatKHR surfaceFormat = surfaceFormats[0];
    for (auto& f : surfaceFormats) {
        if (f.format == VK_FORMAT_B8G8R8A8_UNORM && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            surfaceFormat = f;
            break;
        }
    }

    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (auto& p : presentModes) {
        if (p == VK_PRESENT_MODE_MAILBOX_KHR) {
            presentMode = p;
            break;
        }
    }

    VkExtent2D extent = capabilities.currentExtent;
    if (capabilities.currentExtent.width == INT32_MAX) {
        extent.width = std::max(std::min(WIDTH, capabilities.maxImageExtent.width),
            capabilities.minImageExtent.width);
        extent.height = std::max(std::min(HEIGHT, capabilities.maxImageExtent.height),
            capabilities.minImageExtent.height);
    }

    auto imgCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0) imgCount = std::min(imgCount, capabilities.maxImageCount);

    VkSwapchainCreateInfoKHR swapInfo = {};
    swapInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapInfo.surface = surface;
    swapInfo.minImageCount = imgCount;
    swapInfo.imageFormat = surfaceFormat.format;
    swapInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapInfo.imageExtent = extent;
    swapInfo.imageArrayLayers = 1;
    swapInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (graphicsQueue != presentQueue) {
        swapInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapInfo.queueFamilyIndexCount = 2;
        uint32_t indices[] = { graphicsFamily, graphicsFamily };
        swapInfo.pQueueFamilyIndices = indices;
    }
    else {
        swapInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    swapInfo.preTransform = capabilities.currentTransform;
    swapInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapInfo.presentMode = presentMode;
    swapInfo.clipped = VK_TRUE;
    swapInfo.oldSwapchain = VK_NULL_HANDLE;

    VKDO(vkCreateSwapchainKHR(device, &swapInfo, nullptr, &swapchain));

    uint32_t swapImgCnt;
    vkGetSwapchainImagesKHR(device, swapchain, &swapImgCnt, nullptr);
    swapchainImages.resize(swapImgCnt);
    vkGetSwapchainImagesKHR(device, swapchain, &swapImgCnt, swapchainImages.data());

    FNCOK
}

void VulkanAPI::DestroySurface() {
    vkDestroySurfaceKHR(instance, surface, nullptr);
}

void VulkanAPI::Exit() {
    vkDestroySwapchainKHR(device, swapchain, nullptr);
    vkDestroyDevice(device, nullptr);

    vkDestroyInstance(instance, nullptr);
}