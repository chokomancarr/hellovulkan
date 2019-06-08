#include <iostream>

#include "vulkanapi.hpp"

const int WIDTH = 800;
const int HEIGHT = 600;

void initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    VulkanAPI::window = glfwCreateWindow(WIDTH, HEIGHT, "Hello Vulkan", 0, 0);

}

int main() {
    initWindow();
    VulkanAPI::Init();
    VulkanAPI::CreateSurface();
    VulkanAPI::InitDevice();

    while (!glfwWindowShouldClose(VulkanAPI::window)) {
        glfwPollEvents();
    }

    VulkanAPI::DestroySurface();
    VulkanAPI::Exit();
    glfwDestroyWindow(VulkanAPI::window);
    glfwTerminate();
}