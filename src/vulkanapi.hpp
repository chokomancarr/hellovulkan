#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class VulkanAPI {
public:
    static GLFWwindow* window;

    static void Init();
    static void CreateSurface();
    static void InitDevice();
    static void CreateSwapchain();
    static void CreateImageViews();
    static void CreateRenderPass();
    static void CreateGraphicsPipeline();

    static void DestroySurface();
    static void Exit();
};