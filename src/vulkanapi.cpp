#include "vulkanapi.hpp"
#include <iostream>
#include <algorithm>
#include <set>
#include <vector>
#include <cstdint>
#include "filereader.hpp"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const uint32_t MAX_FRAMES_IN_FLIGHT = 2;

VkResult result;

#define VKDO(cmd) if ((result = cmd) != VK_SUCCESS) {\
    std::cerr << #cmd << " failed with error code " << (int)result << std::endl;\
    abort();\
}
#define FNCOK std::cout << __func__ << "() ok" << std::endl;

GLFWwindow* Vulkan::window;

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
std::vector<VkFramebuffer> swapchainFramebuffers;
std::vector<VkCommandBuffer> commandBuffers;
VkRenderPass renderPass;
VkPipelineLayout pipelineLayout;
VkPipeline pipeline;
VkCommandPool commandPool;
VkSemaphore imgReadySemaphore[MAX_FRAMES_IN_FLIGHT];
VkSemaphore rendFinSemaphore[MAX_FRAMES_IN_FLIGHT];
VkFence rendFences[MAX_FRAMES_IN_FLIGHT];
std::vector<VkFence> imagesInFlight;

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

void Vulkan::Init() {
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

void Vulkan::InitDevice() {
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

void Vulkan::CreateSurface() {
    VKDO(glfwCreateWindowSurface(instance, window, nullptr, &surface));

    FNCOK
}

void Vulkan::CreateSwapchain() {
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

void Vulkan::CreateImageViews() {
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

void Vulkan::CreateRenderPass() {
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

    VkSubpassDependency dep = {};
    dep.srcSubpass = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass = 0;
    dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.srcAccessMask = 0;
    dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo passInfo = {};
    passInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    passInfo.attachmentCount = 1;
    passInfo.pAttachments = &colorAtt;
    passInfo.subpassCount = 1;
    passInfo.pSubpasses = &subpass;
    passInfo.dependencyCount = 1;
    passInfo.pDependencies = &dep;

    VKDO(vkCreateRenderPass(device, &passInfo, nullptr, &renderPass));


    FNCOK
}

void Vulkan::CreateGraphicsPipeline() {
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
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
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

void Vulkan::CreateFramebuffers() {
    for (auto& v : swapchainImageViews) {
        VkImageView atts[] = { v };

        VkFramebufferCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        info.renderPass = renderPass;
        info.attachmentCount = 1;
        info.pAttachments = atts;
        info.width = extent.width;
        info.height = extent.height;
        info.layers = 1;

        VkFramebuffer fbo;
        VKDO(vkCreateFramebuffer(device, &info, nullptr, &fbo));

        swapchainFramebuffers.push_back(fbo);
    }

    FNCOK
}

void Vulkan::CreateCommandPool() {
    VkCommandPoolCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.queueFamilyIndex = graphicsFamily;
    info.flags = 0;
    VKDO(vkCreateCommandPool(device, &info, nullptr, &commandPool));

    FNCOK
}

void Vulkan::CreateCommandBuffers() {
    const auto n = (uint32_t)swapchainFramebuffers.size();

    VkCommandBufferAllocateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.commandPool = commandPool;
    info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    info.commandBufferCount = n;
    commandBuffers.resize(n);
    VKDO(vkAllocateCommandBuffers(device, &info, commandBuffers.data()));

    VkCommandBufferBeginInfo binfo = {};
    binfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    binfo.flags = 0;
    for (uint32_t a = 0; a < n; a++) {
        auto& buf = commandBuffers[a];
        VKDO(vkBeginCommandBuffer(buf, &binfo));
        VkRenderPassBeginInfo pinfo = {};
        pinfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        pinfo.renderPass = renderPass;
        pinfo.framebuffer = swapchainFramebuffers[a];
        pinfo.renderArea.extent = extent;
        VkClearValue cv = {};
        cv.color = {{ 0.f, 0.f, 1.f, 1.f }};
        pinfo.clearValueCount = 1;
        pinfo.pClearValues = &cv;
        vkCmdBeginRenderPass(buf, &pinfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        vkCmdDraw(buf, 3, 1, 0, 0);

        vkCmdEndRenderPass(buf);
        VKDO(vkEndCommandBuffer(buf));
    }
}

void Vulkan::CreateSemaphores() {
    VkSemaphoreCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo finfo = {};
    finfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    finfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t a = 0; a < MAX_FRAMES_IN_FLIGHT; a++) {
        VKDO(vkCreateSemaphore(device, &info, nullptr, &imgReadySemaphore[a]));
        VKDO(vkCreateSemaphore(device, &info, nullptr, &rendFinSemaphore[a]));
        VKDO(vkCreateFence(device, &finfo, nullptr, &rendFences[a]));
    }
    imagesInFlight.resize(swapchainImages.size(), VK_NULL_HANDLE);
}

uint32_t currentFrame = 0;

void Vulkan::DrawFrame() {
    uint32_t id;
    vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imgReadySemaphore[currentFrame], VK_NULL_HANDLE, &id);
    
    auto& fence = rendFences[currentFrame];

    if (imagesInFlight[id] != VK_NULL_HANDLE) {
        vkWaitForFences(device, 1, &imagesInFlight[id], VK_TRUE, UINT64_MAX);
    }
    imagesInFlight[id] = fence;

    vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &fence);

    VkSubmitInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkSemaphore waitSema[] = { imgReadySemaphore[currentFrame] };
    VkPipelineStageFlags flags[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = waitSema;
    info.pWaitDstStageMask = flags;
    info.commandBufferCount = 1;
    info.pCommandBuffers = &commandBuffers[id];
    VkSemaphore sigSema[] = { rendFinSemaphore[currentFrame] };
    info.signalSemaphoreCount = 1;
    info.pSignalSemaphores = sigSema;

    VKDO(vkQueueSubmit(graphicsQueue, 1, &info, fence));

    VkPresentInfoKHR presInfo = {};
    presInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presInfo.waitSemaphoreCount = 1;
    presInfo.pWaitSemaphores = sigSema;
    VkSwapchainKHR swapchains[] = { swapchain };
    presInfo.swapchainCount = 1;
    presInfo.pSwapchains = swapchains;
    presInfo.pImageIndices = &id;
    vkQueuePresentKHR(presentQueue, &presInfo);

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Vulkan::Exit() {
    vkDeviceWaitIdle(device);
    for (uint32_t a = 0; a < MAX_FRAMES_IN_FLIGHT; a++) {
        vkDestroyFence(device, rendFences[a], nullptr);
        vkDestroySemaphore(device, rendFinSemaphore[a], nullptr);
        vkDestroySemaphore(device, imgReadySemaphore[a], nullptr);
    }
    vkDestroyCommandPool(device, commandPool, nullptr);
    for (auto f : swapchainFramebuffers) {
        vkDestroyFramebuffer(device, f, nullptr);
    }
    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);
    for (auto v : swapchainImageViews) {
        vkDestroyImageView(device, v, nullptr);
    }
    vkDestroySwapchainKHR(device, swapchain, nullptr); //<- bad instruction here sometimes
    vkDestroyDevice(device, nullptr);

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
}