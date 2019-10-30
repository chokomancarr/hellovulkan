#include "vulkanapi.hpp"
#include <iostream>
#include <algorithm>
#include <set>
#include <vector>
#include <cstdint>
#include "filereader.hpp"

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
VkSurfaceFormatKHR surfaceFormat;
VkExtent2D extent;
VkSwapchainKHR swapchain;
std::vector<VkImage> swapchainImages;
std::vector<VkImageView> swapchainImageViews;
VkRenderPass renderPass;
VkPipelineLayout pipelineLayout;
VkPipeline pipeline;

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

    surfaceFormat = surfaceFormats[0];
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

    extent = capabilities.currentExtent;
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

    CreateImageViews();
}

void VulkanAPI::CreateImageViews() {
    for (const auto& image : swapchainImages) {
        VkImageViewCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        info.image = image;
        info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        info.format = surfaceFormat.format;
        info.components = {
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY
        };
        info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        info.subresourceRange.baseMipLevel = 0;
        info.subresourceRange.levelCount = 1;
        info.subresourceRange.baseArrayLayer = 0;
        info.subresourceRange.layerCount = 1;
        
        VkImageView view;
        VKDO(vkCreateImageView(device, &info, nullptr, &view));

        swapchainImageViews.push_back(view);
    }

    FNCOK
}

VkShaderModule CreateShaderModule(const std::vector<char>& code) {
    VkShaderModuleCreateInfo info;
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = code.size();
    info.pCode = (const uint32_t*)code.data();
    VkShaderModule mod;
    VKDO(vkCreateShaderModule(device, &info, nullptr, &mod));
    return mod;
}

void VulkanAPI::CreateRenderPass() {
    VkAttachmentDescription colorAtt = {};
    colorAtt.format = surfaceFormat.format;
    colorAtt.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAtt.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAtt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAtt.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAtt.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAtt.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAtt.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference attRef = {};
    attRef.attachment = 0;
    attRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &attRef;

    VkRenderPassCreateInfo passInfo = {};
    passInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    passInfo.attachmentCount = 1;
    passInfo.pAttachments = &colorAtt;
    passInfo.subpassCount = 1;
    passInfo.pSubpasses = &subpass;

    VKDO(vkCreateRenderPass(device, &passInfo, nullptr, &renderPass));

    FNCOK
}

void VulkanAPI::CreateGraphicsPipeline() {
    auto vertCode = FileReader::ReadBytes("tri_v.spv");
    auto fragCode = FileReader::ReadBytes("tri_f.spv");

    auto vert = CreateShaderModule(vertCode);
    auto frag = CreateShaderModule(fragCode);

    VkPipelineShaderStageCreateInfo stages[2] = {};

    auto& infov = stages[0];
    infov.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    infov.stage = VK_SHADER_STAGE_VERTEX_BIT;
    infov.module = vert;
    infov.pName = "main";

    auto& infof = stages[1];
    infof.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    infof.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    infof.module = frag;
    infof.pName = "main";

    VkPipelineVertexInputStateCreateInfo inputInfo = {};
    inputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    inputInfo.vertexBindingDescriptionCount = 0;
    inputInfo.vertexAttributeDescriptionCount = 0;

    VkPipelineInputAssemblyStateCreateInfo assemInfo = {};
    assemInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    assemInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    assemInfo.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = extent.width;
    viewport.height = extent.height;
    viewport.minDepth = 0;
    viewport.maxDepth = 1;

    VkRect2D scissor;
    scissor.offset = { 0, 0 };
    scissor.extent = extent;

    VkPipelineViewportStateCreateInfo viewportInfo = {};
    viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportInfo.viewportCount = 1;
    viewportInfo.pViewports = &viewport;
    viewportInfo.scissorCount = 1;
    viewportInfo.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rastInfo = {};
    rastInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rastInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rastInfo.lineWidth = 1;
    rastInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rastInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo msaaInfo = {};
    msaaInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    msaaInfo.sampleShadingEnable = VK_FALSE;
    msaaInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    msaaInfo.minSampleShading = 1;

    VkPipelineColorBlendAttachmentState blendState = {};
    blendState.colorWriteMask = 
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_A_BIT;
    blendState.blendEnable = VK_TRUE;
    blendState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blendState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendState.colorBlendOp = VK_BLEND_OP_ADD;
    blendState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blendState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    blendState.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo blendInfo = {};
    blendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blendInfo.logicOpEnable = VK_FALSE;
    blendInfo.attachmentCount = 1;
    blendInfo.pAttachments = &blendState;

    //dynamic state here

    VkPipelineLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    VKDO(vkCreatePipelineLayout(device, &layoutInfo, nullptr, &pipelineLayout));

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = stages;
    pipelineInfo.pVertexInputState = &inputInfo;
    pipelineInfo.pInputAssemblyState = &assemInfo;
    pipelineInfo.pViewportState = &viewportInfo;
    pipelineInfo.pRasterizationState = &rastInfo;
    pipelineInfo.pMultisampleState = &msaaInfo;
    pipelineInfo.pColorBlendState = &blendInfo;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    VKDO(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline));

    vkDestroyShaderModule(device, vert, nullptr);
    vkDestroyShaderModule(device, frag, nullptr);

    FNCOK
}

void VulkanAPI::DestroySurface() {
    vkDestroySurfaceKHR(instance, surface, nullptr);
}

void VulkanAPI::Exit() {
    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);
    for (auto v : swapchainImageViews) {
        vkDestroyImageView(device, v, nullptr);
    }
    vkDestroySwapchainKHR(device, swapchain, nullptr);
    vkDestroyDevice(device, nullptr);

    vkDestroyInstance(instance, nullptr);
}