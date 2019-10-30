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
    Vulkan::Init();
    Vulkan::CreateSurface();
    Vulkan::InitDevice();
    Vulkan::CreateSwapchain();
    Vulkan::CreateRenderPass();
    Vulkan::CreateGraphicsPipeline();
    Vulkan::CreateFramebuffers();
    Vulkan::CreateCommandPool();
    Vulkan::CreateCommandBuffers();
    Vulkan::CreateSemaphores();

    while (!glfwWindowShouldClose(Vulkan::window)) {
        glfwPollEvents();
        Vulkan::DrawFrame();
    }

    Vulkan::Exit();
    glfwDestroyWindow(Vulkan::window);
    glfwTerminate();
}