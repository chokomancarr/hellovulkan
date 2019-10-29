#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class VulkanAPI {
public:
    static GLFWwindow* window;

    static void CheckExtSupport();

    static void Init();
    static void InitDevice();
    static void CreateSurface();
    static void DestroySurface();

    static void Exit();
};