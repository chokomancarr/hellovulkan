#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Vulkan {
public:
    static GLFWwindow* window;

    static void Init();
    static void CreateSurface();
    static void InitDevice();
    static void CreateSwapchain();
    static void CreateImageViews();
    static void CreateRenderPass();
    static void CreateGraphicsPipeline();
    static void CreateFramebuffers();
    static void CreateCommandPool();
    static void CreateCommandBuffers();
    static void CreateSemaphores();

    static void DrawFrame();

    static void Exit();
};