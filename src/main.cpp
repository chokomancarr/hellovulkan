#include <iostream>
#include <vector>

#include "vulkanapi.hpp"

void error_callback(int code, const char* description)
{
    std::cerr << "glfw error " << code << ": " << description << std::endl;
}

void initWindow() {
    glfwInit();
    if (!glfwVulkanSupported())
        abort();

    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> props(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, props.data());
    std::cout << "Supported extensions:" << std::endl;
    for (auto& p : props) {
        std::cout << "  " << p.extensionName << std::endl;
    }
        
    glfwSetErrorCallback(error_callback);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
}

int main() {
    initWindow();
    VulkanAPI::Init();
    VulkanAPI::CreateSurface();
    VulkanAPI::InitDevice();
    VulkanAPI::CreateSwapchain();

    while (!glfwWindowShouldClose(VulkanAPI::window)) {
        glfwPollEvents();
    }

    VulkanAPI::DestroySurface();
    VulkanAPI::Exit();
    glfwDestroyWindow(VulkanAPI::window);
    glfwTerminate();
}