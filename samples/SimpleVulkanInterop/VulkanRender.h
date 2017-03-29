//
// Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#define VK_USE_PLATFORM_WIN32_KHR
// Load functions dynamically
#define VK_NO_PROTOTYPES

// This example requires the VK_SDK_PATH environment
// variable to be set as is done with an install of 
// the Lunarg Vulkan SDK package.
#include <vulkan/vulkan.h>

#include "../../inc/LiquidVR.h"
#include "../common/inc/LvrLogger.h"
#include <windef.h>
#include <vector>
#include <fstream>
#include <iostream>

class VulkanRender
{
public:
	VulkanRender( bool renderOffScreen, unsigned width, unsigned height );
	~VulkanRender();

	bool Draw();

    // Get objects
    VkInstance      GetVkInstance() const;
    VkDevice        GetVkDevice() const;
    VkSurfaceKHR    GetVkSurface() const;
    VkQueue			GetVkQueue() const;
    uint32_t		GetVkQueueID() const;
    int             GetCurrentSwapChainImageIndex() const;
    VkImage         GetCurrentSwapChainImage() const;
    
    bool Init();

private:
	bool Shutdown();
	ALVR_RESULT InitVKFnPtr(HMODULE vulkanDLL);
	ALVR_RESULT InitVKFnExtPtr();

	// Vulkan Functions
	VkResult		InitVulkan();
	VkResult		CreateInstance();
	VkResult		CreateDevice();
	VkResult		GetPhysicalDevice();
	VkResult		CreateLogicalDevice();
	VkResult		CreateSwapChain();
	VkResult		CreatePipeline();
	VkResult		CreateRenderPass();
	VkResult		CreateFrameBuffers();
	VkResult		CreateCommands();
	VkResult		CreateSemaphores();
	VkResult		CreatePipelineInput();
	VkResult		CreateVertexBuffers();
	VkResult		CreateStagingBuffer();
	VkResult		CreateDescriptorSetLayout();
	VkResult		CreateDescriptorSetPool();

	// Vulkan Helper functions
	VkResult		LoadShaders();
	VkResult		MakeBuffer(VkDeviceSize size, VkBufferUsageFlags use, VkMemoryPropertyFlags props, VkBuffer& buffer, VkDeviceMemory& memory);
	VkResult		UpdateMVP();
	VkResult		BuildMVP();

	// Vulkan members
 	HMODULE						    m_hVulkanDLL = NULL;

	VkInstance						m_vkInstance = VK_NULL_HANDLE;
	VkDevice						m_vkDevice = VK_NULL_HANDLE;
	VkPhysicalDevice				m_vkPhysicalDevice = VK_NULL_HANDLE;
	VkSurfaceKHR					m_vkSurfaceKHR = VK_NULL_HANDLE;
	VkQueue							m_presentQueue = VK_NULL_HANDLE;
	VkPipeline						m_vkPipeline = VK_NULL_HANDLE;
	VkPipelineLayout				m_vkPipelineLayout = VK_NULL_HANDLE;
	uint32_t						m_presentQueueIdx = VK_NULL_HANDLE;
	VkSwapchainKHR					m_vkSwapChain = VK_NULL_HANDLE;
	VkRenderPass					m_vkRenderPass = VK_NULL_HANDLE;
	VkCommandPool					m_vkCommandPool = VK_NULL_HANDLE;
	VkSemaphore						m_imageGetSemaphore = VK_NULL_HANDLE;
	VkSemaphore						m_imageReadySemaphore = VK_NULL_HANDLE;
	VkDescriptorPool				m_vkDescriptorPool = VK_NULL_HANDLE;
	VkDescriptorSet					m_vkDescriptorSet = VK_NULL_HANDLE;

	std::vector<VkImage>			m_swapChainImages;
	std::vector<VkImageView>		m_SwapChainImageViews;
	std::vector<VkFramebuffer>		m_swapChainFrameBuffers;
	std::vector<VkCommandBuffer>	m_vkCommandBuffers;
	VkFormat						m_swapChainImageFormat;
	VkExtent2D						m_swapChainImageExtent;

	std::vector<char>				m_VertexShader;
	std::vector<char>				m_FragShader;

	// Cube data
	struct Vertex{
		float pos[3];
		float uv[2];
	};

	struct ModelView {
		float model[16];
		float view[16];
		float proj[16];
	};

	float rotationAngle = 0.0f;

	// Vulkan shader settings and data
	VkVertexInputBindingDescription					m_vertexBindingDesc;
	VkDescriptorSetLayout							m_uniformLayout;
	std::vector<VkVertexInputAttributeDescription>	m_vertexAttribDesc;
	std::vector<Vertex>								m_vertexes;
	std::vector<uint32_t>							m_indexes;
	ModelView										m_MVP;
	VkBuffer						                m_vertexBuffer;
	VkDeviceMemory					                m_vertexMemory;
	VkBuffer						                m_indexBuffer;
	VkDeviceMemory					                m_indexMemory;
	VkBuffer						                m_MVPBuffer;
	VkDeviceMemory					                m_MVPMemory;

    bool                                            m_renderOffScreen;
    unsigned                                        m_Width;
    unsigned                                        m_Height;
    int                                             m_CurrentSwapChainImageIndex;
};

// Vulkan function pointers
#ifdef VULKAN_FUNCTIONS
extern PFN_vkCreateInstance vkCreateInstance = NULL;
extern PFN_vkDestroyInstance vkDestroyInstance = NULL;
extern PFN_vkEnumerateInstanceLayerProperties vkEnumerateInstanceLayerProperties = NULL;
extern PFN_vkEnumerateInstanceExtensionProperties vkEnumerateInstanceExtensionProperties = NULL;
extern PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = NULL;
extern PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices = NULL;
extern PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties = NULL;
extern PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties = NULL;
extern PFN_vkCreateDevice vkCreateDevice = NULL;
extern PFN_vkDestroyDevice vkDestroyDevice = NULL;
extern PFN_vkGetDeviceQueue vkGetDeviceQueue = NULL;
extern PFN_vkCreateCommandPool vkCreateCommandPool = NULL;
extern PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers = NULL;
extern PFN_vkCreateFence vkCreateFence = NULL;
extern PFN_vkBeginCommandBuffer vkBeginCommandBuffer = NULL;
extern PFN_vkCmdPipelineBarrier vkCmdPipelineBarrier = NULL;
extern PFN_vkEndCommandBuffer vkEndCommandBuffer = NULL;
extern PFN_vkQueueSubmit vkQueueSubmit = NULL;
extern PFN_vkWaitForFences vkWaitForFences = NULL;
extern PFN_vkResetFences vkResetFences = NULL;
extern PFN_vkResetCommandBuffer vkResetCommandBuffer = NULL;
extern PFN_vkCreateImageView vkCreateImageView = NULL;
extern PFN_vkCreateImage vkCreateImage = NULL;
extern PFN_vkGetImageMemoryRequirements vkGetImageMemoryRequirements = NULL;
extern PFN_vkGetPhysicalDeviceMemoryProperties vkGetPhysicalDeviceMemoryProperties = NULL;
extern PFN_vkAllocateMemory vkAllocateMemory = NULL;
extern PFN_vkBindImageMemory vkBindImageMemory = NULL;
extern PFN_vkCreateRenderPass vkCreateRenderPass = NULL;
extern PFN_vkCreateFramebuffer vkCreateFramebuffer = NULL;
extern PFN_vkCreateBuffer vkCreateBuffer = NULL;
extern PFN_vkDestroyBuffer vkDestroyBuffer = NULL;
extern PFN_vkGetBufferMemoryRequirements vkGetBufferMemoryRequirements = NULL;
extern PFN_vkMapMemory vkMapMemory = NULL;
extern PFN_vkUnmapMemory vkUnmapMemory = NULL;
extern PFN_vkBindBufferMemory vkBindBufferMemory = NULL;
extern PFN_vkCreateShaderModule vkCreateShaderModule = NULL;
extern PFN_vkCreatePipelineLayout vkCreatePipelineLayout = NULL;
extern PFN_vkCreateGraphicsPipelines vkCreateGraphicsPipelines = NULL;
extern PFN_vkCreateSemaphore vkCreateSemaphore = NULL;
extern PFN_vkCmdBeginRenderPass vkCmdBeginRenderPass = NULL;
extern PFN_vkCmdBindPipeline vkCmdBindPipeline = NULL;
extern PFN_vkCmdSetViewport vkCmdSetViewport = NULL;
extern PFN_vkCmdSetScissor vkCmdSetScissor = NULL;
extern PFN_vkCmdBindVertexBuffers vkCmdBindVertexBuffers = NULL;
extern PFN_vkCmdDraw vkCmdDraw = NULL;
extern PFN_vkCmdEndRenderPass vkCmdEndRenderPass = NULL;
extern PFN_vkDestroyFence vkDestroyFence = NULL;
extern PFN_vkDestroySemaphore vkDestroySemaphore = NULL;
extern PFN_vkCreateDescriptorSetLayout vkCreateDescriptorSetLayout = NULL;
extern PFN_vkCreateDescriptorPool vkCreateDescriptorPool = NULL;
extern PFN_vkAllocateDescriptorSets vkAllocateDescriptorSets = NULL;
extern PFN_vkUpdateDescriptorSets vkUpdateDescriptorSets = NULL;
extern PFN_vkCmdBindDescriptorSets vkCmdBindDescriptorSets = NULL;
extern PFN_vkFlushMappedMemoryRanges vkFlushMappedMemoryRanges = NULL;
extern PFN_vkCreateSampler vkCreateSampler = NULL;
extern PFN_vkDestroyImageView vkDestroyImageView = NULL;
extern PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR = NULL;
extern PFN_vkDestroyShaderModule vkDestroyShaderModule = NULL;
extern PFN_vkDestroyPipelineLayout vkDestroyPipelineLayout = NULL;
extern PFN_vkDestroyRenderPass vkDestroyRenderPass = NULL;
extern PFN_vkCmdCopyBuffer vkCmdCopyBuffer = NULL;
extern PFN_vkQueueWaitIdle vkQueueWaitIdle = NULL;
extern PFN_vkFreeMemory vkFreeMemory = NULL;
extern PFN_vkFreeCommandBuffers vkFreeCommandBuffers = NULL;
extern PFN_vkCmdCopyImage vkCmdCopyImage = NULL;
extern PFN_vkDestroyImage vkDestroyImage = NULL;
extern PFN_vkCmdCopyImageToBuffer vkCmdCopyImageToBuffer = NULL;
extern PFN_vkCmdCopyBufferToImage vkCmdCopyBufferToImage = NULL;
extern PFN_vkEnumerateDeviceExtensionProperties vkEnumerateDeviceExtensionProperties = NULL;
extern PFN_vkDestroyFramebuffer vkDestroyFramebuffer = NULL;
extern PFN_vkDestroyCommandPool vkDestroyCommandPool = NULL;
extern PFN_vkDeviceWaitIdle vkDeviceWaitIdle = NULL;
extern PFN_vkCmdBindIndexBuffer vkCmdBindIndexBuffer = NULL;
extern PFN_vkCmdDrawIndexed vkCmdDrawIndexed = NULL;
extern PFN_vkDestroyDescriptorSetLayout vkDestroyDescriptorSetLayout = NULL;
extern PFN_vkDestroyDescriptorPool vkDestroyDescriptorPool = NULL;

extern PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT = NULL;
extern PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT = NULL;
extern PFN_vkDebugReportMessageEXT vkDebugReportMessageEXT = NULL;

// Windows platform:
extern PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR = NULL;
extern PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR = NULL;
extern PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR = NULL;
extern PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR = NULL;
extern PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR = NULL;
extern PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR = NULL;

// Swapchain extension:
extern PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR = NULL;
extern PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR = NULL;
extern PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR = NULL;
extern PFN_vkQueuePresentKHR vkQueuePresentKHR = NULL; 
#else
extern PFN_vkCreateInstance vkCreateInstance;
extern PFN_vkCreateInstance vkCreateInstance;
extern PFN_vkDestroyInstance vkDestroyInstance;
extern PFN_vkEnumerateInstanceLayerProperties vkEnumerateInstanceLayerProperties;
extern PFN_vkEnumerateInstanceExtensionProperties vkEnumerateInstanceExtensionProperties;
extern PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
extern PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices;
extern PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties;
extern PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties;
extern PFN_vkCreateDevice vkCreateDevice;
extern PFN_vkDestroyDevice vkDestroyDevice;
extern PFN_vkGetDeviceQueue vkGetDeviceQueue;
extern PFN_vkCreateCommandPool vkCreateCommandPool;
extern PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers;
extern PFN_vkCreateFence vkCreateFence;
extern PFN_vkBeginCommandBuffer vkBeginCommandBuffer;
extern PFN_vkCmdPipelineBarrier vkCmdPipelineBarrier;
extern PFN_vkEndCommandBuffer vkEndCommandBuffer;
extern PFN_vkQueueSubmit vkQueueSubmit;
extern PFN_vkWaitForFences vkWaitForFences;
extern PFN_vkResetFences vkResetFences;
extern PFN_vkResetCommandBuffer vkResetCommandBuffer;
extern PFN_vkCreateImageView vkCreateImageView;
extern PFN_vkCreateImage vkCreateImage;
extern PFN_vkGetImageMemoryRequirements vkGetImageMemoryRequirements;
extern PFN_vkGetPhysicalDeviceMemoryProperties vkGetPhysicalDeviceMemoryProperties;
extern PFN_vkAllocateMemory vkAllocateMemory;
extern PFN_vkBindImageMemory vkBindImageMemory;
extern PFN_vkCreateRenderPass vkCreateRenderPass;
extern PFN_vkCreateFramebuffer vkCreateFramebuffer;
extern PFN_vkCreateBuffer vkCreateBuffer;
extern PFN_vkDestroyBuffer vkDestroyBuffer;
extern PFN_vkGetBufferMemoryRequirements vkGetBufferMemoryRequirements;
extern PFN_vkMapMemory vkMapMemory;
extern PFN_vkUnmapMemory vkUnmapMemory;
extern PFN_vkBindBufferMemory vkBindBufferMemory;
extern PFN_vkCreateShaderModule vkCreateShaderModule;
extern PFN_vkCreatePipelineLayout vkCreatePipelineLayout;
extern PFN_vkCreateGraphicsPipelines vkCreateGraphicsPipelines;
extern PFN_vkCreateSemaphore vkCreateSemaphore;
extern PFN_vkCmdBeginRenderPass vkCmdBeginRenderPass;
extern PFN_vkCmdBindPipeline vkCmdBindPipeline;
extern PFN_vkCmdSetViewport vkCmdSetViewport;
extern PFN_vkCmdSetScissor vkCmdSetScissor;
extern PFN_vkCmdBindVertexBuffers vkCmdBindVertexBuffers;
extern PFN_vkCmdDraw vkCmdDraw;
extern PFN_vkCmdEndRenderPass vkCmdEndRenderPass;
extern PFN_vkDestroyFence vkDestroyFence;
extern PFN_vkDestroySemaphore vkDestroySemaphore;
extern PFN_vkCreateDescriptorSetLayout vkCreateDescriptorSetLayout;
extern PFN_vkCreateDescriptorPool vkCreateDescriptorPool;
extern PFN_vkAllocateDescriptorSets vkAllocateDescriptorSets;
extern PFN_vkUpdateDescriptorSets vkUpdateDescriptorSets;
extern PFN_vkCmdBindDescriptorSets vkCmdBindDescriptorSets;
extern PFN_vkFlushMappedMemoryRanges vkFlushMappedMemoryRanges;
extern PFN_vkCreateSampler vkCreateSampler;
extern PFN_vkDestroyImageView vkDestroyImageView;
extern PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR;
extern PFN_vkDestroyShaderModule vkDestroyShaderModule;
extern PFN_vkDestroyPipelineLayout vkDestroyPipelineLayout;
extern PFN_vkDestroyRenderPass vkDestroyRenderPass;
extern PFN_vkCmdCopyBuffer vkCmdCopyBuffer;
extern PFN_vkQueueWaitIdle vkQueueWaitIdle;
extern PFN_vkFreeMemory vkFreeMemory;
extern PFN_vkFreeCommandBuffers vkFreeCommandBuffers;
extern PFN_vkCmdCopyImage vkCmdCopyImage;
extern PFN_vkDestroyImage vkDestroyImage;
extern PFN_vkCmdCopyImageToBuffer vkCmdCopyImageToBuffer;
extern PFN_vkCmdCopyBufferToImage vkCmdCopyBufferToImage;
extern PFN_vkEnumerateDeviceExtensionProperties vkEnumerateDeviceExtensionProperties;
extern PFN_vkDestroyFramebuffer vkDestroyFramebuffer;
extern PFN_vkDestroyCommandPool vkDestroyCommandPool;
extern PFN_vkDeviceWaitIdle vkDeviceWaitIdle;
extern PFN_vkCmdBindIndexBuffer vkCmdBindIndexBuffer;
extern PFN_vkCmdDrawIndexed vkCmdDrawIndexed;
extern PFN_vkDestroyDescriptorSetLayout vkDestroyDescriptorSetLayout;
extern PFN_vkDestroyDescriptorPool vkDestroyDescriptorPool;

extern PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT;
extern PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT;
extern PFN_vkDebugReportMessageEXT vkDebugReportMessageEXT;

// Windows platform:
extern PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR;
extern PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR;
extern PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR;
extern PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR;
extern PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
extern PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR;

// Swapchain extension:
extern PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR;
extern PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR;
extern PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR;
extern PFN_vkQueuePresentKHR vkQueuePresentKHR; 
#endif
