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
// SimpleVulkanInterop.cpp : Defines the entry point for the console application
//

#include "VulkanRender.h"
#include <DirectXMath.h>

// This list is from the VK_LAYER_LUNARG_standard_validation macro but without the VK_LAYER_LUNARG_object_tracker or the VK_LAYER_GOOGLE_unique_objects
static const std::vector<const char*> validationLayers = { "VK_LAYER_GOOGLE_threading", "VK_LAYER_LUNARG_parameter_validation", /*"VK_LAYER_LUNARG_device_limits",*/ "VK_LAYER_LUNARG_image", "VK_LAYER_LUNARG_core_validation","VK_LAYER_LUNARG_swapchain" };
static const std::vector<const char*> instanceExtensions = { "VK_KHR_surface", "VK_KHR_win32_surface", "VK_EXT_debug_report"};
static const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

// Math needed for cube and MVP, all angle measurements are in Radians
#define ROTATION_SPEED 0.0005f

//-------------------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hwnd,
	UINT msg,
	WPARAM wParam,
	LPARAM lParam)
{
	switch (msg)
	{
	    case WM_DESTROY:
		    PostQuitMessage(0);
		    return 0;
	}
	return DefWindowProc(hwnd,
		msg,
		wParam,
		lParam);
}

// VulkanRender
//

//-------------------------------------------------------------------------------------------------
bool VulkanRender::Init()
{
	VkResult vkResult = InitVulkan();
    if (VK_SUCCESS != vkResult)
    {
        std::cout << "Failed to inialize Vulkan\n";
        return false;
    }
	return true;
}

//-------------------------------------------------------------------------------------------------
ALVR_RESULT VulkanRender::InitVKFnPtr(HMODULE vulkanDLL)
{
	vkCreateInstance = (PFN_vkCreateInstance)GetProcAddress(vulkanDLL, "vkCreateInstance");
	CHECK_RETURN(NULL != vkCreateInstance, ALVR_FAIL, "Failed to find vkCreateInstance");
	vkDestroyInstance = (PFN_vkDestroyInstance)GetProcAddress(vulkanDLL, "vkDestroyInstance");
	CHECK_RETURN(NULL != vkDestroyInstance, ALVR_FAIL, "Failed to find vkDestroyInstance");
	vkEnumerateInstanceLayerProperties = (PFN_vkEnumerateInstanceLayerProperties)GetProcAddress(vulkanDLL, "vkEnumerateInstanceLayerProperties");
	CHECK_RETURN(NULL != vkEnumerateInstanceLayerProperties, ALVR_FAIL, "Failed to find vkEnumerateInstanceLayerProperties");
	vkEnumerateInstanceExtensionProperties = (PFN_vkEnumerateInstanceExtensionProperties)GetProcAddress(vulkanDLL, "vkEnumerateInstanceExtensionProperties");
	CHECK_RETURN(NULL != vkEnumerateInstanceExtensionProperties, ALVR_FAIL, "Failed to find vkEnumerateInstanceExtensionProperties");
	vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)GetProcAddress(vulkanDLL, "vkGetInstanceProcAddr");
	CHECK_RETURN(NULL != vkGetInstanceProcAddr, ALVR_FAIL, "Failed to find vkGetInstanceProcAddr");
	vkEnumeratePhysicalDevices = (PFN_vkEnumeratePhysicalDevices)GetProcAddress(vulkanDLL, "vkEnumeratePhysicalDevices");
	CHECK_RETURN(NULL != vkEnumeratePhysicalDevices, ALVR_FAIL, "Failed to find vkEnumeratePhysicalDevices");
	vkGetPhysicalDeviceProperties = (PFN_vkGetPhysicalDeviceProperties)GetProcAddress(vulkanDLL, "vkGetPhysicalDeviceProperties");
	CHECK_RETURN(NULL != vkGetPhysicalDeviceProperties, ALVR_FAIL, "Failed to find vkGetPhysicalDeviceProperties");
	vkGetPhysicalDeviceQueueFamilyProperties = (PFN_vkGetPhysicalDeviceQueueFamilyProperties)GetProcAddress(vulkanDLL, "vkGetPhysicalDeviceQueueFamilyProperties");
	CHECK_RETURN(NULL != vkGetPhysicalDeviceQueueFamilyProperties, ALVR_FAIL, "Failed to find vkGetPhysicalDeviceQueueFamilyProperties");
	vkCreateDevice = (PFN_vkCreateDevice)GetProcAddress(vulkanDLL, "vkCreateDevice");
	CHECK_RETURN(NULL != vkCreateDevice, ALVR_FAIL, "Failed to find vkCreateDevice");
	vkDestroyDevice = (PFN_vkDestroyDevice)GetProcAddress(vulkanDLL, "vkDestroyDevice");
	CHECK_RETURN(NULL != vkDestroyDevice, ALVR_FAIL, "Failed to find vkDestroyDevice");
	vkGetDeviceQueue = (PFN_vkGetDeviceQueue)GetProcAddress(vulkanDLL, "vkGetDeviceQueue");
	CHECK_RETURN(NULL != vkGetDeviceQueue, ALVR_FAIL, "Failed to find vkGetDeviceQueue");
	vkCreateCommandPool = (PFN_vkCreateCommandPool)GetProcAddress(vulkanDLL, "vkCreateCommandPool");
	CHECK_RETURN(NULL != vkCreateCommandPool, ALVR_FAIL, "Failed to find vkCreateCommandPool");
	vkAllocateCommandBuffers = (PFN_vkAllocateCommandBuffers)GetProcAddress(vulkanDLL, "vkAllocateCommandBuffers");
	CHECK_RETURN(NULL != vkAllocateCommandBuffers, ALVR_FAIL, "Failed to find vkAllocateCommandBuffers");
	vkCreateFence = (PFN_vkCreateFence)GetProcAddress(vulkanDLL, "vkCreateFence");
	CHECK_RETURN(NULL != vkCreateFence, ALVR_FAIL, "Failed to find vkCreateFence");
	vkBeginCommandBuffer = (PFN_vkBeginCommandBuffer)GetProcAddress(vulkanDLL, "vkBeginCommandBuffer");
	CHECK_RETURN(NULL != vkBeginCommandBuffer, ALVR_FAIL, "Failed to find vkBeginCommandBuffer");
	vkCmdPipelineBarrier = (PFN_vkCmdPipelineBarrier)GetProcAddress(vulkanDLL, "vkCmdPipelineBarrier");
	CHECK_RETURN(NULL != vkCmdPipelineBarrier, ALVR_FAIL, "Failed to find vkCmdPipelineBarrier");
	vkEndCommandBuffer = (PFN_vkEndCommandBuffer)GetProcAddress(vulkanDLL, "vkEndCommandBuffer");
	CHECK_RETURN(NULL != vkEndCommandBuffer, ALVR_FAIL, "Failed to find vkEndCommandBuffer");
	vkQueueSubmit = (PFN_vkQueueSubmit)GetProcAddress(vulkanDLL, "vkQueueSubmit");
	CHECK_RETURN(NULL != vkQueueSubmit, ALVR_FAIL, "Failed to find vkQueueSubmit");
	vkWaitForFences = (PFN_vkWaitForFences)GetProcAddress(vulkanDLL, "vkWaitForFences");
	CHECK_RETURN(NULL != vkWaitForFences, ALVR_FAIL, "Failed to find vkWaitForFences");
	vkResetFences = (PFN_vkResetFences)GetProcAddress(vulkanDLL, "vkResetFences");
	CHECK_RETURN(NULL != vkResetFences, ALVR_FAIL, "Failed to find vkResetFences");
	vkResetCommandBuffer = (PFN_vkResetCommandBuffer)GetProcAddress(vulkanDLL, "vkResetCommandBuffer");
	CHECK_RETURN(NULL != vkResetCommandBuffer, ALVR_FAIL, "Failed to find vkResetCommandBuffer");
	vkCreateImageView = (PFN_vkCreateImageView)GetProcAddress(vulkanDLL, "vkCreateImageView");
	CHECK_RETURN(NULL != vkCreateImageView, ALVR_FAIL, "Failed to find vkCreateImageView");
	vkCreateImage = (PFN_vkCreateImage)GetProcAddress(vulkanDLL, "vkCreateImage");
	CHECK_RETURN(NULL != vkCreateImage, ALVR_FAIL, "Failed to find vkCreateImage");
	vkGetImageMemoryRequirements = (PFN_vkGetImageMemoryRequirements)GetProcAddress(vulkanDLL, "vkGetImageMemoryRequirements");
	CHECK_RETURN(NULL != vkGetImageMemoryRequirements, ALVR_FAIL, "Failed to find vkGetImageMemoryRequirements");
	vkGetPhysicalDeviceMemoryProperties = (PFN_vkGetPhysicalDeviceMemoryProperties)GetProcAddress(vulkanDLL, "vkGetPhysicalDeviceMemoryProperties");
	CHECK_RETURN(NULL != vkGetPhysicalDeviceMemoryProperties, ALVR_FAIL, "Failed to find vkGetPhysicalDeviceMemoryProperties");
	vkAllocateMemory = (PFN_vkAllocateMemory)GetProcAddress(vulkanDLL, "vkAllocateMemory");
	CHECK_RETURN(NULL != vkAllocateMemory, ALVR_FAIL, "Failed to find vkAllocateMemory");
	vkBindImageMemory = (PFN_vkBindImageMemory)GetProcAddress(vulkanDLL, "vkBindImageMemory");
	CHECK_RETURN(NULL != vkBindImageMemory, ALVR_FAIL, "Failed to find vkBindImageMemory");
	vkCreateRenderPass = (PFN_vkCreateRenderPass)GetProcAddress(vulkanDLL, "vkCreateRenderPass");
	CHECK_RETURN(NULL != vkCreateRenderPass, ALVR_FAIL, "Failed to find vkCreateRenderPass");
	vkCreateFramebuffer = (PFN_vkCreateFramebuffer)GetProcAddress(vulkanDLL, "vkCreateFramebuffer");
	CHECK_RETURN(NULL != vkCreateFramebuffer, ALVR_FAIL, "Failed to find vkCreateFramebuffer");
	vkCreateBuffer = (PFN_vkCreateBuffer)GetProcAddress(vulkanDLL, "vkCreateBuffer");
	CHECK_RETURN(NULL != vkCreateBuffer, ALVR_FAIL, "Failed to find vkCreateBuffer");
	vkDestroyBuffer = (PFN_vkDestroyBuffer)GetProcAddress(vulkanDLL, "vkDestroyBuffer");
	CHECK_RETURN(NULL != vkDestroyBuffer, ALVR_FAIL, "Failed to find vkDestroyBuffer");
	vkGetBufferMemoryRequirements = (PFN_vkGetBufferMemoryRequirements)GetProcAddress(vulkanDLL, "vkGetBufferMemoryRequirements");
	CHECK_RETURN(NULL != vkGetBufferMemoryRequirements, ALVR_FAIL, "Failed to find vkGetBufferMemoryRequirements");
	vkMapMemory = (PFN_vkMapMemory)GetProcAddress(vulkanDLL, "vkMapMemory");
	CHECK_RETURN(NULL != vkMapMemory, ALVR_FAIL, "Failed to find vkMapMemory");
	vkUnmapMemory = (PFN_vkUnmapMemory)GetProcAddress(vulkanDLL, "vkUnmapMemory");
	CHECK_RETURN(NULL != vkUnmapMemory, ALVR_FAIL, "Failed to find vkUnmapMemory");
	vkBindBufferMemory = (PFN_vkBindBufferMemory)GetProcAddress(vulkanDLL, "vkBindBufferMemory");
	CHECK_RETURN(NULL != vkBindBufferMemory, ALVR_FAIL, "Failed to find vkBindBufferMemory");
	vkCreateShaderModule = (PFN_vkCreateShaderModule)GetProcAddress(vulkanDLL, "vkCreateShaderModule");
	CHECK_RETURN(NULL != vkCreateShaderModule, ALVR_FAIL, "Failed to find vkCreateShaderModule");
	vkCreatePipelineLayout = (PFN_vkCreatePipelineLayout)GetProcAddress(vulkanDLL, "vkCreatePipelineLayout");
	CHECK_RETURN(NULL != vkCreatePipelineLayout, ALVR_FAIL, "Failed to find vkCreatePipelineLayout");
	vkCreateGraphicsPipelines = (PFN_vkCreateGraphicsPipelines)GetProcAddress(vulkanDLL, "vkCreateGraphicsPipelines");
	CHECK_RETURN(NULL != vkCreateGraphicsPipelines, ALVR_FAIL, "Failed to find vkCreateGraphicsPipelines");
	vkCreateSemaphore = (PFN_vkCreateSemaphore)GetProcAddress(vulkanDLL, "vkCreateSemaphore");
	CHECK_RETURN(NULL != vkCreateSemaphore, ALVR_FAIL, "Failed to find vkCreateSemaphore");
	vkCmdBeginRenderPass = (PFN_vkCmdBeginRenderPass)GetProcAddress(vulkanDLL, "vkCmdBeginRenderPass");
	CHECK_RETURN(NULL != vkCmdBeginRenderPass, ALVR_FAIL, "Failed to find vkCmdBeginRenderPass");
	vkCmdBindPipeline = (PFN_vkCmdBindPipeline)GetProcAddress(vulkanDLL, "vkCmdBindPipeline");
	CHECK_RETURN(NULL != vkCmdBindPipeline, ALVR_FAIL, "Failed to find vkCmdBindPipeline");
	vkCmdSetViewport = (PFN_vkCmdSetViewport)GetProcAddress(vulkanDLL, "vkCmdSetViewport");
	CHECK_RETURN(NULL != vkCmdSetViewport, ALVR_FAIL, "Failed to find vkCmdSetViewport");
	vkCmdSetScissor = (PFN_vkCmdSetScissor)GetProcAddress(vulkanDLL, "vkCmdSetScissor");
	CHECK_RETURN(NULL != vkCmdSetScissor, ALVR_FAIL, "Failed to find vkCmdSetScissor");
	vkCmdBindVertexBuffers = (PFN_vkCmdBindVertexBuffers)GetProcAddress(vulkanDLL, "vkCmdBindVertexBuffers");
	CHECK_RETURN(NULL != vkCmdBindVertexBuffers, ALVR_FAIL, "Failed to find vkCmdBindVertexBuffers");
	vkCmdDraw = (PFN_vkCmdDraw)GetProcAddress(vulkanDLL, "vkCmdDraw");
	CHECK_RETURN(NULL != vkCmdDraw, ALVR_FAIL, "Failed to find vkCmdDraw");
	vkCmdEndRenderPass = (PFN_vkCmdEndRenderPass)GetProcAddress(vulkanDLL, "vkCmdEndRenderPass");
	CHECK_RETURN(NULL != vkCmdEndRenderPass, ALVR_FAIL, "Failed to find vkCmdEndRenderPass");
	vkDestroyFence = (PFN_vkDestroyFence)GetProcAddress(vulkanDLL, "vkDestroyFence");
	CHECK_RETURN(NULL != vkDestroyFence, ALVR_FAIL, "Failed to find vkDestroyFence");
	vkDestroySemaphore = (PFN_vkDestroySemaphore)GetProcAddress(vulkanDLL, "vkDestroySemaphore");
	CHECK_RETURN(NULL != vkDestroyFence, ALVR_FAIL, "Failed to find vkDestroyFence");
	vkCreateDescriptorSetLayout = (PFN_vkCreateDescriptorSetLayout)GetProcAddress(vulkanDLL, "vkCreateDescriptorSetLayout");
	CHECK_RETURN(NULL != vkCreateDescriptorSetLayout, ALVR_FAIL, "Failed to find vkCreateDescriptorSetLayout");
	vkCreateDescriptorPool = (PFN_vkCreateDescriptorPool)GetProcAddress(vulkanDLL, "vkCreateDescriptorPool");
	CHECK_RETURN(NULL != vkCreateDescriptorPool, ALVR_FAIL, "Failed to find vkCreateDescriptorPool");
	vkAllocateDescriptorSets = (PFN_vkAllocateDescriptorSets)GetProcAddress(vulkanDLL, "vkAllocateDescriptorSets");
	CHECK_RETURN(NULL != vkAllocateDescriptorSets, ALVR_FAIL, "Failed to find vkAllocateDescriptorSets");
	vkUpdateDescriptorSets = (PFN_vkUpdateDescriptorSets)GetProcAddress(vulkanDLL, "vkUpdateDescriptorSets");
	CHECK_RETURN(NULL != vkUpdateDescriptorSets, ALVR_FAIL, "Failed to find vkUpdateDescriptorSets");
	vkCmdBindDescriptorSets = (PFN_vkCmdBindDescriptorSets)GetProcAddress(vulkanDLL, "vkCmdBindDescriptorSets");
	CHECK_RETURN(NULL != vkCmdBindDescriptorSets, ALVR_FAIL, "Failed to find vkCmdBindDescriptorSets");
	vkFlushMappedMemoryRanges = (PFN_vkFlushMappedMemoryRanges)GetProcAddress(vulkanDLL, "vkFlushMappedMemoryRanges");
	CHECK_RETURN(NULL != vkFlushMappedMemoryRanges, ALVR_FAIL, "Failed to find vkFlushMappedMemoryRanges");
	vkCreateSampler = (PFN_vkCreateSampler)GetProcAddress(vulkanDLL, "vkCreateSampler");
	CHECK_RETURN(NULL != vkCreateSampler, ALVR_FAIL, "Failed to find vkCreateSampler");
	vkCmdCopyBuffer = (PFN_vkCmdCopyBuffer)GetProcAddress(vulkanDLL, "vkCmdCopyBuffer");
	CHECK_RETURN(NULL != vkCmdCopyBuffer, ALVR_FAIL, "Failed to find vkCmdCopyBuffer");
	vkQueueWaitIdle = (PFN_vkQueueWaitIdle)GetProcAddress(vulkanDLL, "vkQueueWaitIdle");
	CHECK_RETURN(NULL != vkQueueWaitIdle, ALVR_FAIL, "Failed to find vkQueueWaitIdle");
	vkFreeMemory = (PFN_vkFreeMemory)GetProcAddress(vulkanDLL, "vkFreeMemory");
	CHECK_RETURN(NULL != vkFreeMemory, ALVR_FAIL, "Failed to find vkFreeMemory");
	vkFreeCommandBuffers = (PFN_vkFreeCommandBuffers)GetProcAddress(vulkanDLL, "vkFreeCommandBuffers");
	CHECK_RETURN(NULL != vkFreeCommandBuffers, ALVR_FAIL, "Failed to find vkFreeCommandBuffers");
	vkCmdCopyImage = (PFN_vkCmdCopyImage)GetProcAddress(vulkanDLL, "vkCmdCopyImage");
	CHECK_RETURN(NULL != vkCmdCopyImage, ALVR_FAIL, "Failed to find vkCmdCopyImage");
	vkDestroyImage = (PFN_vkDestroyImage)GetProcAddress(vulkanDLL, "vkDestroyImage");
	CHECK_RETURN(NULL != vkDestroyImage, ALVR_FAIL, "Failed to find vkDestroyImage");
	vkCmdCopyImageToBuffer = (PFN_vkCmdCopyImageToBuffer)GetProcAddress(vulkanDLL, "vkCmdCopyImageToBuffer");
	CHECK_RETURN(NULL != vkDestroyImage, ALVR_FAIL, "Failed to find vkCmdCopyImageToBuffer");
	vkDestroyImageView = (PFN_vkDestroyImageView)GetProcAddress(vulkanDLL, "vkDestroyImageView");
	CHECK_RETURN(NULL != vkDestroyImageView, ALVR_FAIL, "Failed to find vkDestroyImageView");
	vkDestroySwapchainKHR = (PFN_vkDestroySwapchainKHR)GetProcAddress(vulkanDLL, "vkDestroySwapchainKHR");
	CHECK_RETURN(NULL != vkDestroySwapchainKHR, ALVR_FAIL, "Failed to find vkDestroySwapchainKHR");
	vkDestroyShaderModule = (PFN_vkDestroyShaderModule)GetProcAddress(vulkanDLL, "vkDestroyShaderModule");
	CHECK_RETURN(NULL != vkDestroyShaderModule, ALVR_FAIL, "Failed to find vkDestroyShaderModule");
	vkDestroyPipelineLayout = (PFN_vkDestroyPipelineLayout)GetProcAddress(vulkanDLL, "vkDestroyPipelineLayout");
	CHECK_RETURN(NULL != vkDestroyPipelineLayout, ALVR_FAIL, "Failed to find vkDestroyPipelineLayout");
	vkDestroyRenderPass = (PFN_vkDestroyRenderPass)GetProcAddress(vulkanDLL, "vkDestroyRenderPass");
	CHECK_RETURN(NULL != vkDestroyRenderPass, ALVR_FAIL, "Failed to find vkDestroyRenderPass");
	vkEnumerateDeviceExtensionProperties = (PFN_vkEnumerateDeviceExtensionProperties)GetProcAddress(vulkanDLL, "vkEnumerateDeviceExtensionProperties");
	CHECK_RETURN(NULL != vkEnumerateDeviceExtensionProperties, ALVR_FAIL, "Failed to find vkEnumerateDeviceExtensionProperties");
	vkDestroyFramebuffer = (PFN_vkDestroyFramebuffer)GetProcAddress(vulkanDLL, "vkDestroyFramebuffer");
	CHECK_RETURN(NULL != vkDestroyFramebuffer, ALVR_FAIL, "Failed to find vkDestroyFramebuffer");
	vkDestroyCommandPool = (PFN_vkDestroyCommandPool)GetProcAddress(vulkanDLL, "vkDestroyCommandPool");
	CHECK_RETURN(NULL != vkDestroyCommandPool, ALVR_FAIL, "Failed to find vkDestroyCommandPool");
	vkDeviceWaitIdle = (PFN_vkDeviceWaitIdle)GetProcAddress(vulkanDLL, "vkDeviceWaitIdle");
	CHECK_RETURN(NULL != vkDeviceWaitIdle, ALVR_FAIL, "Failed to find vkDeviceWaitIdle");
	vkCmdBindIndexBuffer = (PFN_vkCmdBindIndexBuffer)GetProcAddress(vulkanDLL, "vkCmdBindIndexBuffer");
	CHECK_RETURN(NULL != vkCmdBindIndexBuffer, ALVR_FAIL, "Failed to find vkCmdBindIndexBuffer");
	vkCmdDrawIndexed = (PFN_vkCmdDrawIndexed)GetProcAddress(vulkanDLL, "vkCmdDrawIndexed");
	CHECK_RETURN(NULL != vkCmdDrawIndexed, ALVR_FAIL, "Failed to find vkCmdDrawIndexed");
	vkDestroyDescriptorSetLayout = (PFN_vkDestroyDescriptorSetLayout)GetProcAddress(vulkanDLL, "vkDestroyDescriptorSetLayout");
	CHECK_RETURN(NULL != vkDestroyDescriptorSetLayout, ALVR_FAIL, "Failed to find vkDestroyDescriptorSetLayout");
	vkDestroyDescriptorPool = (PFN_vkDestroyDescriptorPool)GetProcAddress(vulkanDLL, "vkDestroyDescriptorPool");
	CHECK_RETURN(NULL != vkDestroyDescriptorPool, ALVR_FAIL, "Failed to find vkDestroyDescriptorPool");
    vkCmdCopyBufferToImage = (PFN_vkCmdCopyBufferToImage)GetProcAddress(vulkanDLL, "vkCmdCopyBufferToImage");
    CHECK_RETURN(NULL != vkCmdCopyBufferToImage, ALVR_FAIL, "Failed to find vkCmdCopyBufferToImage");

	return ALVR_OK;
}

//-------------------------------------------------------------------------------------------------
ALVR_RESULT VulkanRender::InitVKFnExtPtr()
{
	*(void **)&vkCreateDebugReportCallbackEXT = vkGetInstanceProcAddr(m_vkInstance, "vkCreateDebugReportCallbackEXT");
	CHECK_RETURN(NULL != vkCreateDebugReportCallbackEXT, ALVR_FAIL, "Failed to find vkCreateDebugReportCallbackEXT");
	*(void **)&vkDestroyDebugReportCallbackEXT = vkGetInstanceProcAddr(m_vkInstance, "vkDestroyDebugReportCallbackEXT");
	CHECK_RETURN(NULL != vkDestroyDebugReportCallbackEXT, ALVR_FAIL, "Failed to find vkDestroyDebugReportCallbackEXT");
	*(void **)&vkDebugReportMessageEXT = vkGetInstanceProcAddr(m_vkInstance, "vkDebugReportMessageEXT");
	CHECK_RETURN(NULL != vkDebugReportMessageEXT, ALVR_FAIL, "Failed to find vkDebugReportMessageEXT");
	*(void **)&vkCreateWin32SurfaceKHR = vkGetInstanceProcAddr(m_vkInstance, "vkCreateWin32SurfaceKHR");
	CHECK_RETURN(NULL != vkCreateWin32SurfaceKHR, ALVR_FAIL, "Failed to find vkCreateWin32SurfaceKHR");
	*(void **)&vkDestroySurfaceKHR = vkGetInstanceProcAddr(m_vkInstance, "vkDestroySurfaceKHR");
	CHECK_RETURN(NULL != vkDestroySurfaceKHR, ALVR_FAIL, "Failed to find vkDestroySurfaceKHR");
	*(void **)&vkGetPhysicalDeviceSurfaceSupportKHR = vkGetInstanceProcAddr(m_vkInstance, "vkGetPhysicalDeviceSurfaceSupportKHR");
	CHECK_RETURN(NULL != vkGetPhysicalDeviceSurfaceSupportKHR, ALVR_FAIL, "Failed to find vkGetPhysicalDeviceSurfaceSupportKHR");
	*(void **)&vkGetPhysicalDeviceSurfaceFormatsKHR = vkGetInstanceProcAddr(m_vkInstance, "vkGetPhysicalDeviceSurfaceFormatsKHR");
	CHECK_RETURN(NULL != vkGetPhysicalDeviceSurfaceFormatsKHR, ALVR_FAIL, "Failed to find vkGetPhysicalDeviceSurfaceFormatsKHR");
	*(void **)&vkGetPhysicalDeviceSurfaceCapabilitiesKHR = vkGetInstanceProcAddr(m_vkInstance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
	CHECK_RETURN(NULL != vkGetPhysicalDeviceSurfaceCapabilitiesKHR, ALVR_FAIL, "Failed to find vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
	*(void **)&vkGetPhysicalDeviceSurfacePresentModesKHR = vkGetInstanceProcAddr(m_vkInstance, "vkGetPhysicalDeviceSurfacePresentModesKHR");
	CHECK_RETURN(NULL != vkGetPhysicalDeviceSurfacePresentModesKHR, ALVR_FAIL, "Failed to find vkGetPhysicalDeviceSurfacePresentModesKHR");
	*(void **)&vkCreateSwapchainKHR = vkGetInstanceProcAddr(m_vkInstance, "vkCreateSwapchainKHR");
	CHECK_RETURN(NULL != vkCreateSwapchainKHR, ALVR_FAIL, "Failed to find vkCreateSwapchainKHR");
	*(void **)&vkGetSwapchainImagesKHR = vkGetInstanceProcAddr(m_vkInstance, "vkGetSwapchainImagesKHR");
	CHECK_RETURN(NULL != vkGetSwapchainImagesKHR, ALVR_FAIL, "Failed to find vkGetSwapchainImagesKHR");
	*(void **)&vkAcquireNextImageKHR = vkGetInstanceProcAddr(m_vkInstance, "vkAcquireNextImageKHR");
	CHECK_RETURN(NULL != vkAcquireNextImageKHR, ALVR_FAIL, "Failed to find vkAcquireNextImageKHR");
	*(void **)&vkQueuePresentKHR = vkGetInstanceProcAddr(m_vkInstance, "vkQueuePresentKHR");

	return ALVR_OK;
}

//-------------------------------------------------------------------------------------------------
VkResult VulkanRender::InitVulkan()
{
	VkResult res = VK_INCOMPLETE;

	m_hVulkanDLL = LoadLibrary(L"vulkan-1.dll");

	InitVKFnPtr(m_hVulkanDLL);
	if (m_hVulkanDLL == NULL)
	{
		return VK_INCOMPLETE;
	}
	res = CreateInstance();
	if (res != VK_SUCCESS)
	{
		return res;
	}
	InitVKFnExtPtr();
	res = CreateDevice();
	if (res != VK_SUCCESS)
	{
		return res;
	}
	res = CreateSwapChain();
	if (res != VK_SUCCESS)
	{
		return res;
	}
	res = CreateRenderPass();
	if (res != VK_SUCCESS)
	{
		return res;
	}
	res = CreatePipelineInput();
	if (res != VK_SUCCESS)
	{
		return res;
	}
	res = CreateDescriptorSetLayout();
	if (res != VK_SUCCESS)
	{
		return res;
	}
	res = CreatePipeline();
	if (res != VK_SUCCESS)
	{
		return res;
	}
	res = CreateFrameBuffers();
	if (res != VK_SUCCESS)
	{
		return res;
	}
	res = CreateCommands();
	if (res != VK_SUCCESS)
	{
		return res;
	}
	res = CreateSemaphores();
	if (res != VK_SUCCESS)
	{
		return res;
	}

	return VK_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
VulkanRender::VulkanRender(bool renderOffScreen, unsigned width, unsigned height)
    : m_renderOffScreen(renderOffScreen), m_CurrentSwapChainImageIndex(-1),
    m_Width(width), m_Height(height)
{
}

//-------------------------------------------------------------------------------------------------
VulkanRender::~VulkanRender()
{
	Shutdown();
}

//-------------------------------------------------------------------------------------------------
bool VulkanRender::Shutdown()
{
	// Destroy everything in reverse order
	if (m_vkDevice != NULL)
	{
		if (m_vertexBuffer != NULL)
		{
			vkDestroyBuffer(m_vkDevice, m_vertexBuffer, nullptr);
		}
		if (m_indexBuffer != NULL)
		{
			vkDestroyBuffer(m_vkDevice, m_indexBuffer, nullptr);
		}
        if (m_MVPBuffer != NULL)
        {
            vkDestroyBuffer(m_vkDevice, m_MVPBuffer, nullptr);
        }
		if (m_vertexMemory != NULL)
		{
			vkFreeMemory(m_vkDevice, m_vertexMemory, nullptr);
		}
        if (m_indexMemory != NULL)
        {
            vkFreeMemory(m_vkDevice, m_indexMemory, nullptr);
        }
        if (m_MVPMemory != NULL)
        {
            vkFreeMemory(m_vkDevice, m_MVPMemory, nullptr);
        }
        if (m_imageReadySemaphore != NULL){
			vkDestroySemaphore(m_vkDevice, m_imageReadySemaphore, nullptr);
		}
		if (m_imageGetSemaphore)
		{
			vkDestroySemaphore(m_vkDevice, m_imageGetSemaphore, nullptr);
		}
		if (m_vkDescriptorPool != NULL)
		{
			vkDestroyDescriptorPool(m_vkDevice, m_vkDescriptorPool, nullptr);
		}
        if (m_vkCommandBuffers.size() > 0)
        {
            for (unsigned i = 0; i < m_vkCommandBuffers.size(); i++)
            {
                vkFreeCommandBuffers(m_vkDevice, m_vkCommandPool,1,&m_vkCommandBuffers[i]);
            }
        }
        if (m_vkCommandPool != NULL)
		{
			vkDestroyCommandPool(m_vkDevice, m_vkCommandPool, nullptr);
		}
		for (auto frameBuffer : m_swapChainFrameBuffers)
        {
			if (frameBuffer != NULL)
			{
				vkDestroyFramebuffer(m_vkDevice, frameBuffer, nullptr);
			}
		}
		if (m_vkRenderPass != NULL)
		{
			vkDestroyRenderPass(m_vkDevice, m_vkRenderPass, nullptr);
		}
		if (m_vkPipeline != NULL)
		{
			vkDestroyPipelineLayout(m_vkDevice, m_vkPipelineLayout, nullptr);
		}
		if (m_uniformLayout != NULL)
        {
			vkDestroyDescriptorSetLayout(m_vkDevice, m_uniformLayout, nullptr);
		}
        for (auto vkImageView : m_SwapChainImageViews)
		{
			if (vkImageView != NULL)
			{
				vkDestroyImageView(m_vkDevice, vkImageView, nullptr);
			}
		}
		if (m_vkSwapChain != NULL)
		{
			vkDestroySwapchainKHR(m_vkDevice, m_vkSwapChain, nullptr);
		}
		if (m_vkSurfaceKHR != NULL)
		{
			vkDestroySurfaceKHR(m_vkInstance, m_vkSurfaceKHR, nullptr);
		}
		vkDestroyDevice(m_vkDevice, nullptr);
	}
	vkDestroyInstance(m_vkInstance, nullptr);

	return true;
}

//-------------------------------------------------------------------------------------------------
VkResult VulkanRender::CreateInstance()
{
	VkResult res = VK_INCOMPLETE;

	VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.apiVersion = VK_API_VERSION_1_0;
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pApplicationName = "SimpleVulkan";
		appInfo.pEngineName = "None";
		appInfo.pNext = nullptr;

	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

	uint32_t foundExtensions = 0;
	for (uint32_t i = 0; i < extensionCount; i++)
	{
        for (uint32_t j = 0; j < instanceExtensions.size(); j++)
        {
            if (strcmp(extensions[i].extensionName, instanceExtensions[j]) == 0)
            {
                foundExtensions++;
				break;
            }
        }
	}
	// Check return on foundextensions
	if (foundExtensions < instanceExtensions.size())
    {
		return VK_INCOMPLETE;
	}

    std::vector<VkLayerProperties> layers;
#ifdef WANT_VALIDATION
	uint32_t layerCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    layers.resize(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, layers.data());

	for (auto layerName : validationLayers)
    {
		bool missingLayer = true;
		for (auto layerProp : layers)
        {
			if (strcmp(layerProp.layerName,layerName) == 0)
			{
				missingLayer = false;
			}
		}
		if (missingLayer)
		{
			return VK_ERROR_LAYER_NOT_PRESENT;
		}
	}
#endif

	VkInstanceCreateInfo creationInfo = {};
		creationInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        creationInfo.enabledExtensionCount = (uint32_t) instanceExtensions.size();
		creationInfo.enabledLayerCount = 
            layers.empty() ? 0 : (uint32_t) validationLayers.size();
		creationInfo.pApplicationInfo = &appInfo;
		creationInfo.pNext = nullptr;
        creationInfo.ppEnabledExtensionNames = instanceExtensions.data();
		creationInfo.ppEnabledLayerNames = 
            layers.empty() ? nullptr : validationLayers.data();

	res = vkCreateInstance(&creationInfo, nullptr, &m_vkInstance);
	if (res != VK_SUCCESS)
	{
		return res;
	}

	return VK_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
VkResult VulkanRender::CreateDevice(){
	VkResult res = VK_INCOMPLETE;

	res = GetPhysicalDevice();
	if (res != VK_SUCCESS)
	{
		return res;
	}
	res = CreateLogicalDevice();
	if (res != VK_SUCCESS)
	{
		return res;
	}

	return VK_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
VkResult VulkanRender::GetPhysicalDevice()
{
	VkResult res = VK_INCOMPLETE;
	
	uint32_t deviceCount = 0;
	VkPhysicalDeviceProperties physicalDeviceProps;

	// Windows Specific
	HWND windowHandle = NULL;
	HINSTANCE hInstance = (HINSTANCE)GetModuleHandle(NULL);

    WNDCLASSEX wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = NULL;
    wcex.cbWndExtra = NULL;
    wcex.hInstance = hInstance;
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hIcon = NULL;
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = L"Vulkan Render";
    wcex.hIconSm = NULL;

    RegisterClassEx(&wcex);

    windowHandle = CreateWindowEx(NULL,
        L"Vulkan Render",
        L"Vulkan Render",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        m_Width, m_Height,
        NULL,
        NULL,
        hInstance,
        NULL);

    if (!windowHandle)
    {
        return VK_INCOMPLETE;
    }

    if (!m_renderOffScreen)
    {
        ShowWindow(windowHandle, SW_NORMAL);
        UpdateWindow(windowHandle);
    }

	// Create the windows surface
	VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
		surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		surfaceCreateInfo.hinstance = hInstance;
		surfaceCreateInfo.hwnd = windowHandle;
		surfaceCreateInfo.pNext = nullptr;

	res = vkCreateWin32SurfaceKHR(m_vkInstance, &surfaceCreateInfo, nullptr, &m_vkSurfaceKHR);
	if (res != VK_SUCCESS)
	{
		return res;
	}

	res = vkEnumeratePhysicalDevices(m_vkInstance, &deviceCount, nullptr);
	if (deviceCount == 0)
	{
		return VK_ERROR_DEVICE_LOST;
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	res = vkEnumeratePhysicalDevices(m_vkInstance, &deviceCount, devices.data());
	if (res != VK_SUCCESS)
	{
		return res;
	}

	// Check for a device
	for (auto device : devices)
	{
		VkPhysicalDeviceProperties deviceProps = {};
		vkGetPhysicalDeviceProperties(device, &deviceProps);

		uint32_t queueFamilycount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilycount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilycount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilycount, queueFamilyProperties.data());

		// Just take the first one that has the graphics option
		for (uint32_t i = 0; i < queueFamilycount; i++)
        {
			VkBool32 presentSupport = false;
			res = vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_vkSurfaceKHR, &presentSupport);
			if (presentSupport && queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				m_vkPhysicalDevice = device;
				physicalDeviceProps = deviceProps;
				m_presentQueueIdx = i;
				break;
			}
		}
		if (m_vkPhysicalDevice != VK_NULL_HANDLE)
		{
			break;
		}
	}
	if (m_vkPhysicalDevice == VK_NULL_HANDLE)
	{
		return VK_ERROR_INITIALIZATION_FAILED;
	}
	return VK_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
VkResult VulkanRender::CreateLogicalDevice()
{ 
	VkResult res = VK_INCOMPLETE;
	
	VkPhysicalDeviceFeatures deviceFeats = {};
	uint32_t deviceExtensionCount = 0;
	res = vkEnumerateDeviceExtensionProperties(m_vkPhysicalDevice, nullptr, &deviceExtensionCount, nullptr);
	if (res != VK_SUCCESS)
	{
		return res;
	}
	std::vector<VkExtensionProperties> availableExtensions(deviceExtensionCount);
	res = vkEnumerateDeviceExtensionProperties(m_vkPhysicalDevice, nullptr, &deviceExtensionCount, availableExtensions.data());
	for (auto deviceEx : deviceExtensions)
    {
		bool extensionMissing = true;
		for (auto availEx : availableExtensions)
        {
			if (strcmp(availEx.extensionName, deviceEx) == 0)
			{
				extensionMissing = false;
				break;
			}
		}
		if (extensionMissing)
        {
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

	float pQueuePriorities = 1.0;
	VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.pQueuePriorities = &pQueuePriorities;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.queueFamilyIndex = m_presentQueueIdx;
		queueCreateInfo.pNext = nullptr;

	VkDeviceCreateInfo deviceCreateInfo = {};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
		deviceCreateInfo.queueCreateInfoCount = 1;
        deviceCreateInfo.enabledLayerCount = (uint32_t) validationLayers.size();
		deviceCreateInfo.pEnabledFeatures = &deviceFeats;
		deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
        deviceCreateInfo.enabledExtensionCount = (uint32_t) deviceExtensions.size();
		deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

	res = vkCreateDevice(m_vkPhysicalDevice, &deviceCreateInfo, nullptr, &m_vkDevice);
	if (res != VK_SUCCESS)
	{
		return res;
	}

	vkGetDeviceQueue(m_vkDevice, m_presentQueueIdx, 0, &m_presentQueue);
	if (m_presentQueue == VK_NULL_HANDLE)
	{
		return VK_INCOMPLETE;
	}

	return VK_SUCCESS; 
}

//-------------------------------------------------------------------------------------------------
VkResult VulkanRender::CreateSwapChain()
{
	VkResult res = VK_INCOMPLETE;

	VkSurfaceCapabilitiesKHR surfaceCapabilities = {};
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentMode;

	res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_vkPhysicalDevice, m_vkSurfaceKHR, &surfaceCapabilities);
	if (res != VK_SUCCESS)
	{
		return res;
	}

	uint32_t formatCount, presentModeCount = 0;
	res = vkGetPhysicalDeviceSurfaceFormatsKHR(m_vkPhysicalDevice, m_vkSurfaceKHR, &formatCount, nullptr);
	if (formatCount == 0 || res != VK_SUCCESS)
	{
		return VK_INCOMPLETE;
	}
	formats.resize(formatCount);
	res = vkGetPhysicalDeviceSurfaceFormatsKHR(m_vkPhysicalDevice, m_vkSurfaceKHR, &formatCount, formats.data());
	if (res != VK_SUCCESS)
	{
		return res;
	}

	res = vkGetPhysicalDeviceSurfacePresentModesKHR(m_vkPhysicalDevice, m_vkSurfaceKHR, &presentModeCount, nullptr);
	if (presentModeCount == 0 || res != VK_SUCCESS)
	{
		return VK_INCOMPLETE;
	}
	presentMode.resize(presentModeCount);
	res = vkGetPhysicalDeviceSurfacePresentModesKHR(m_vkPhysicalDevice, m_vkSurfaceKHR, &presentModeCount, presentMode.data());
	if (res != VK_SUCCESS)
	{
		return res;
	}

	uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
	if (surfaceCapabilities.maxImageCount > 0 && imageCount > surfaceCapabilities.maxImageCount)
	{
		imageCount = surfaceCapabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
		swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapChainCreateInfo.surface = m_vkSurfaceKHR;
		swapChainCreateInfo.minImageCount = imageCount;
		swapChainCreateInfo.imageExtent = surfaceCapabilities.currentExtent;
		swapChainCreateInfo.imageArrayLayers = 1;
		swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; //Potentially change for compute queue
		swapChainCreateInfo.queueFamilyIndexCount = 1;
		swapChainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
		swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapChainCreateInfo.clipped = VK_TRUE;
		swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;
		swapChainCreateInfo.pQueueFamilyIndices = &m_presentQueueIdx;

	for (auto format : formats)
    {
		if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			swapChainCreateInfo.imageFormat = format.format;
			swapChainCreateInfo.imageColorSpace = format.colorSpace;
			m_swapChainImageFormat = format.format;
			break;
		}
	}

	for (auto present : presentMode)
    {
		if (present == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			swapChainCreateInfo.presentMode = present;
		}
	}

	// Create swap chain
	res = vkCreateSwapchainKHR(m_vkDevice, &swapChainCreateInfo, nullptr, &m_vkSwapChain);
	if (res != VK_SUCCESS)
	{
		return res;
	}

	// Store images in the chain
	imageCount = 0;
	res = vkGetSwapchainImagesKHR(m_vkDevice, m_vkSwapChain, &imageCount, nullptr);
	if (res != VK_SUCCESS)
	{
		return res;
	}
	m_swapChainImages.resize(imageCount);
	res = vkGetSwapchainImagesKHR(m_vkDevice, m_vkSwapChain, &imageCount, m_swapChainImages.data());
	if (res != VK_SUCCESS)
	{
		return res;
	}

	m_swapChainImageExtent = surfaceCapabilities.currentExtent;

	m_SwapChainImageViews.resize(m_swapChainImages.size());
	for (uint32_t i = 0; i < m_swapChainImages.size(); i++)
    {
		VkImageViewCreateInfo imageViewCreateInfo = {};
			imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			imageViewCreateInfo.image = m_swapChainImages[i];
			imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			imageViewCreateInfo.format = m_swapChainImageFormat;
	
			imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	
			imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
			imageViewCreateInfo.subresourceRange.levelCount = 1;
			imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
			imageViewCreateInfo.subresourceRange.layerCount = 1;

		res = vkCreateImageView(m_vkDevice, &imageViewCreateInfo, nullptr, &m_SwapChainImageViews[i]);
		if (res != VK_SUCCESS)
		{
			return res;
		}
	}

	return VK_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
VkResult VulkanRender::CreatePipeline()
{
	VkResult res = VK_INCOMPLETE;

	VkShaderModule vertModule;
	VkShaderModule fragModule;

	res = LoadShaders();
    if (res != VK_SUCCESS)
    {
        return res;
    }

	VkShaderModuleCreateInfo vertModuleCreateInfo = {};
		vertModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		vertModuleCreateInfo.codeSize = m_VertexShader.size();
		vertModuleCreateInfo.pCode = (uint32_t*)m_VertexShader.data();

	VkShaderModuleCreateInfo fragModuleCreateInfo = {};
		fragModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		fragModuleCreateInfo.codeSize = m_FragShader.size();
		fragModuleCreateInfo.pCode = (uint32_t*)m_FragShader.data();

	res = vkCreateShaderModule(m_vkDevice, &vertModuleCreateInfo, nullptr, &vertModule);
	if (res != VK_SUCCESS)
	{
		return res;
	}
	res = vkCreateShaderModule(m_vkDevice, &fragModuleCreateInfo, nullptr, &fragModule);
	if (res != VK_SUCCESS)
	{
		return res;
	}

	VkPipelineShaderStageCreateInfo vertStageInfo = {};
		vertStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertStageInfo.module = vertModule;
		vertStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragStageInfo = {};
		fragStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragStageInfo.module = fragModule;
		fragStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertStageInfo, fragStageInfo };

	// Fixed Stages
	VkPipelineVertexInputStateCreateInfo vertInputInfo = {};
		vertInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertInputInfo.vertexBindingDescriptionCount = 1;
		vertInputInfo.pVertexBindingDescriptions = &m_vertexBindingDesc;
        vertInputInfo.vertexAttributeDescriptionCount = (uint32_t) m_vertexAttribDesc.size();
		vertInputInfo.pVertexAttributeDescriptions = m_vertexAttribDesc.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssemply = {};
		inputAssemply.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemply.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssemply.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)m_swapChainImageExtent.width;
		viewport.height = (float)m_swapChainImageExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = m_swapChainImageExtent;

	VkPipelineViewportStateCreateInfo viewportState = {};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizerInfo = {};
		rasterizerInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizerInfo.depthClampEnable = VK_TRUE;
		rasterizerInfo.rasterizerDiscardEnable = VK_FALSE;
		rasterizerInfo.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizerInfo.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizerInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizerInfo.depthBiasEnable = VK_FALSE;
		rasterizerInfo.lineWidth = 1.0f;

	VkPipelineMultisampleStateCreateInfo multisampling = {};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState colorBlendAtt = {}; // Sets up color / alpha blending.
		colorBlendAtt.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAtt.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo blendInfo = {};
		blendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		blendInfo.logicOpEnable = VK_FALSE;
		blendInfo.logicOp = VK_LOGIC_OP_COPY;
		blendInfo.attachmentCount = 1;
		blendInfo.pAttachments = &colorBlendAtt;

	VkDescriptorSetLayout setLayouts[] = {m_uniformLayout};
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = setLayouts;

	res = vkCreatePipelineLayout(m_vkDevice, &pipelineLayoutInfo, nullptr, &m_vkPipelineLayout);
	if (res != VK_SUCCESS)
	{
		return res;
	}

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssemply;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizerInfo;
		pipelineInfo.pDepthStencilState = nullptr;
		pipelineInfo.pColorBlendState = &blendInfo;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.layout = m_vkPipelineLayout;
		pipelineInfo.renderPass = m_vkRenderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	res = vkCreateGraphicsPipelines(m_vkDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_vkPipeline);
	if (res != VK_SUCCESS)
	{
		return res;
	}


	// Cleanup
	vkDestroyShaderModule(m_vkDevice, vertModule, nullptr);
	vkDestroyShaderModule(m_vkDevice, fragModule, nullptr);

	return VK_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
VkResult VulkanRender::CreateRenderPass()
{
	VkResult res = VK_INCOMPLETE;

	VkAttachmentDescription colorAtt = {};
		colorAtt.format = m_swapChainImageFormat;
		colorAtt.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAtt.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAtt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAtt.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAtt.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		colorAtt.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAtt.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	VkAttachmentReference colorAttRef = {};
		colorAttRef.attachment = 0;
		colorAttRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subPassDesc = {};
		subPassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subPassDesc.colorAttachmentCount = 1;
		subPassDesc.pColorAttachments = &colorAttRef;
	
	VkSubpassDependency subDep = {};
		subDep.srcSubpass = VK_SUBPASS_EXTERNAL;
		subDep.dstSubpass = 0;
		subDep.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		subDep.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		subDep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subDep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	
	VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAtt;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subPassDesc;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &subDep;

	res = vkCreateRenderPass(m_vkDevice, &renderPassInfo, nullptr, &m_vkRenderPass);
	if (res != VK_SUCCESS)
	{
		return res;
	}

	return VK_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
VkResult VulkanRender::CreateFrameBuffers()
{
	VkResult res = VK_INCOMPLETE;

	m_swapChainFrameBuffers.resize(m_SwapChainImageViews.size());

	for (uint32_t i = 0; i < m_SwapChainImageViews.size(); i++)
    {
		VkImageView attachments[] = {
			m_SwapChainImageViews[i]
		};

		VkFramebufferCreateInfo frameBufferInfo = {};
			frameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			frameBufferInfo.renderPass = m_vkRenderPass;
			frameBufferInfo.attachmentCount = 1;
			frameBufferInfo.pAttachments = &m_SwapChainImageViews[i];
			frameBufferInfo.width = m_swapChainImageExtent.width;
			frameBufferInfo.height = m_swapChainImageExtent.height;
			frameBufferInfo.layers = 1;

		res = vkCreateFramebuffer(m_vkDevice, &frameBufferInfo, nullptr, &m_swapChainFrameBuffers[i]);
		if (res != VK_SUCCESS)
		{
			return res;
		}
	}

	return VK_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
VkResult VulkanRender::CreateCommands()
{
	VkResult res = VK_INCOMPLETE;

	VkCommandPoolCreateInfo poolCreateInfo = {};
		poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolCreateInfo.queueFamilyIndex = m_presentQueueIdx;

	res = vkCreateCommandPool(m_vkDevice, &poolCreateInfo, nullptr, &m_vkCommandPool);
	if (res != VK_SUCCESS){
		return res;
	}

	res = CreateVertexBuffers();
	if (res != VK_SUCCESS)
	{
		return res;
	}
	res = CreateDescriptorSetPool();
	if (res != VK_SUCCESS)
	{
		return res;
	}

	// Command buffers
	m_vkCommandBuffers.resize(m_swapChainFrameBuffers.size());

	VkCommandBufferAllocateInfo bufferAllocInfo = {};
		bufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		bufferAllocInfo.commandPool = m_vkCommandPool;
		bufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		bufferAllocInfo.commandBufferCount = (uint32_t)m_vkCommandBuffers.size();

	res = vkAllocateCommandBuffers(m_vkDevice, &bufferAllocInfo, m_vkCommandBuffers.data());
	if (res != VK_SUCCESS)
	{
		return res;
	}

	for (uint32_t i = 0; i < m_vkCommandBuffers.size(); i++)
    {
		VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

		vkBeginCommandBuffer(m_vkCommandBuffers[i], &beginInfo);

		VkClearValue clearColor = { 1, 1, 1, 1 };
		VkRenderPassBeginInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = m_vkRenderPass;
			renderPassInfo.framebuffer = m_swapChainFrameBuffers[i];
			renderPassInfo.renderArea.offset = { 0, 0 };
			renderPassInfo.renderArea.extent = m_swapChainImageExtent;
			renderPassInfo.clearValueCount = 1;
			renderPassInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(m_vkCommandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(m_vkCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_vkPipeline);

		VkBuffer vertexBuffers[] = { m_vertexBuffer };
		VkDeviceSize offsets[] = {0};
		vkCmdBindVertexBuffers(m_vkCommandBuffers[i], 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(m_vkCommandBuffers[i], m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdBindDescriptorSets(m_vkCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_vkPipelineLayout, 0, 1, &m_vkDescriptorSet, 0, nullptr);

        vkCmdDrawIndexed(m_vkCommandBuffers[i], (uint32_t) m_indexes.size(), 1, 0, 0, 0);

		vkCmdEndRenderPass(m_vkCommandBuffers[i]);
		res = vkEndCommandBuffer(m_vkCommandBuffers[i]);
		if (res != VK_SUCCESS)
        {
			return res;
		}
	}

	return VK_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
VkResult VulkanRender::LoadShaders()
{
	VkResult res = VK_INCOMPLETE;


    wchar_t path[5 * MAX_PATH];
    ::GetModuleFileNameW(NULL, path, _countof(path));
    std::wstring filepath(path);
    std::wstring::size_type slash = filepath.find_last_of(L"\\/");
    filepath = filepath.substr(0, slash + 1);
    std::wstring fileName = filepath + L"vert.spv";
	//Load data from file
	std::ifstream vertFile(fileName, std::ios::ate | std::ios::binary);
    fileName = filepath + L"frag.spv";
	std::ifstream fragFile(fileName, std::ios::ate | std::ios::binary);

	if (!vertFile.is_open() || !fragFile.is_open())
	{
		return res;
	}
	size_t vertFileSize = (size_t)vertFile.tellg();
	size_t fragFileSize = (size_t)fragFile.tellg();
	m_VertexShader.resize(vertFileSize);
	m_FragShader.resize(fragFileSize);
	vertFile.seekg(0);
	fragFile.seekg(0);
	vertFile.read(m_VertexShader.data(), vertFileSize);
	fragFile.read(m_FragShader.data(), fragFileSize);

	vertFile.close();
	fragFile.close();

	return VK_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
VkResult VulkanRender::CreateSemaphores()
{
	VkResult res = VK_INCOMPLETE;

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	res = vkCreateSemaphore(m_vkDevice, &semaphoreInfo, nullptr, &m_imageGetSemaphore);
	if (res != VK_SUCCESS)
	{
		return res;
	}
	res = vkCreateSemaphore(m_vkDevice, &semaphoreInfo, nullptr, &m_imageReadySemaphore);
	if (res != VK_SUCCESS)
	{
		return res;
	}

	return VK_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
bool VulkanRender::Draw()
{
	VkResult res = VK_INCOMPLETE;

	// Get Image from Swapchain
	uint32_t imageIndex = 0;
	vkAcquireNextImageKHR(m_vkDevice, m_vkSwapChain, UINT_MAX, m_imageGetSemaphore, VK_NULL_HANDLE, &imageIndex);

	VkSemaphore waitSemaphores[] = { m_imageGetSemaphore };
	VkSemaphore signalSemaphores[] = { m_imageReadySemaphore };

	VkPipelineStageFlags waitFlags[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitFlags;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_vkCommandBuffers[imageIndex];
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

	res = vkQueueSubmit(m_presentQueue, 1, &submitInfo, VK_NULL_HANDLE);
	if (res != VK_SUCCESS)
	{
		return false;
	}

	// Present image
	VkSwapchainKHR swapChains[] = { m_vkSwapChain };
	VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;

	vkQueuePresentKHR(m_presentQueue, &presentInfo);
	vkDeviceWaitIdle(m_vkDevice);
	UpdateMVP();

    // Set the current image index as the host application may need this
    m_CurrentSwapChainImageIndex = imageIndex;

	return true;
}

//-------------------------------------------------------------------------------------------------
VkResult VulkanRender::CreatePipelineInput()
{
	VkResult res = VK_INCOMPLETE;

	m_vertexBindingDesc = {};
	m_vertexBindingDesc.binding = 0;
	m_vertexBindingDesc.stride = sizeof(Vertex);
	m_vertexBindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	m_vertexAttribDesc.resize(2); // One for each attribute in the vertex struct; Pos/UV

	// Position
	m_vertexAttribDesc[0].binding = 0;
	m_vertexAttribDesc[0].location = 0;
	m_vertexAttribDesc[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	m_vertexAttribDesc[0].offset = offsetof(Vertex, pos);

	// UV
	m_vertexAttribDesc[1].binding = 0;
	m_vertexAttribDesc[1].location = 1;
	m_vertexAttribDesc[1].format = VK_FORMAT_R32G32_SFLOAT;
	m_vertexAttribDesc[1].offset = offsetof(Vertex, uv);

	return VK_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
VkResult VulkanRender::CreateVertexBuffers()
{
	VkResult res = VK_INCOMPLETE;

	// Vertex data
	m_vertexes = 
    {
		//Front
		{ { 1, 1, 1 }, { 1, 0 } },  // Right Bottom
		{ { 1, -1, 1 }, { 1, 1 } }, // Right top
		{ { -1, 1, 1 }, { 0, 0 } }, // Left Bottom
		{ { -1, -1, 1 }, { 0, 1 } }, // Left Top
		//Back
		{ { 1, 1, -1 }, { 0, 1 } }, // Right Bottom
		{ { 1, -1, -1 }, { 0, 0 } }, // Right top
		{ { -1, 1, -1 }, { 1, 1 } }, // Left Bottom
		{ { -1, -1, -1 }, { 1, 0 } } // Left Top
	};

	// Index data
	m_indexes = 
    {
		//Front
		0, 1, 2,
		3, 2, 1,
		//Right
		4, 5, 0,
		1, 0, 5,
		//left
		2, 3, 6,
		3, 7, 6,
		//back
		4, 6, 7,
		4, 7, 5,
		//top
		3, 1, 5,
		5, 7, 3,
		//bottom
		6, 4, 0,
		0, 2, 6
	};

	// MVP data
	BuildMVP();

	MakeBuffer(sizeof(Vertex) * m_vertexes.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_vertexBuffer, m_vertexMemory);
	MakeBuffer(sizeof(uint32_t) * m_indexes.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_indexBuffer, m_indexMemory);
	MakeBuffer(sizeof(ModelView), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_MVPBuffer, m_MVPMemory);

	// vertex
	void* bufferData = NULL;
	res = vkMapMemory(m_vkDevice, m_vertexMemory, 0, sizeof(Vertex) * m_vertexes.size(), 0, &bufferData);
	if (res != VK_SUCCESS || bufferData == NULL)
	{
		return res;
	}
	memcpy(bufferData, m_vertexes.data(), (size_t)sizeof(Vertex) * m_vertexes.size());
	vkUnmapMemory(m_vkDevice, m_vertexMemory);
	bufferData = NULL;

	// index
	res = vkMapMemory(m_vkDevice, m_indexMemory, 0, sizeof(uint32_t) * m_indexes.size(), 0, &bufferData);
	if (res != VK_SUCCESS || bufferData == NULL)
	{
		return res;
	}
	memcpy(bufferData, m_indexes.data(), (size_t)sizeof(uint32_t) * m_indexes.size());
	vkUnmapMemory(m_vkDevice, m_indexMemory);
	bufferData = NULL;

	// mvp
	res = vkMapMemory(m_vkDevice, m_MVPMemory, 0, sizeof(ModelView), 0, &bufferData);
	if (res != VK_SUCCESS || bufferData == NULL)
	{
		return res;
	}
	memcpy(bufferData, &m_MVP, sizeof(ModelView));
	vkUnmapMemory(m_vkDevice, m_MVPMemory);
	bufferData = NULL;

	return VK_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
VkResult VulkanRender::MakeBuffer(VkDeviceSize size, VkBufferUsageFlags use, VkMemoryPropertyFlags props, VkBuffer& buffer, VkDeviceMemory& memory){
	VkResult res = VK_INCOMPLETE;
	
	VkBufferCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		createInfo.size = size;
		createInfo.usage = use;
		createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	res = vkCreateBuffer(m_vkDevice, &createInfo, nullptr, &buffer);
	if (res != VK_SUCCESS)
    {
		return res;
	}

	VkMemoryRequirements memReqs = {};
	vkGetBufferMemoryRequirements(m_vkDevice, buffer, &memReqs);

	VkPhysicalDeviceMemoryProperties memProps;
	vkGetPhysicalDeviceMemoryProperties(m_vkPhysicalDevice, &memProps);

	uint32_t memType = -1;
	for (uint32_t i = 0; i < memProps.memoryTypeCount; i++)
    {
		if (memProps.memoryTypes[i].propertyFlags & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
        {
			if ((memProps.memoryTypes[i].propertyFlags & memReqs.memoryTypeBits))
			{
				memType = i;
				break;
			}
		}
	}

	if (memType == ((uint32_t)-1))
    {
		return VK_ERROR_DEVICE_LOST;
	}

	VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memReqs.size;
		allocInfo.memoryTypeIndex = memType;

	res = vkAllocateMemory(m_vkDevice, &allocInfo, nullptr, &memory);
	if (res != VK_SUCCESS)
	{
		return res;
	}
	vkBindBufferMemory(m_vkDevice, buffer, memory, 0);

	return VK_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
VkResult VulkanRender::CreateDescriptorSetLayout()
{
	VkResult res = VK_INCOMPLETE;

	VkDescriptorSetLayoutBinding MVPLayoutBinding = {};
		MVPLayoutBinding.binding = 0;
		MVPLayoutBinding.descriptorCount = 1;
		MVPLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		MVPLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
		layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutCreateInfo.bindingCount = 1;
		layoutCreateInfo.pBindings = &MVPLayoutBinding;

	res = vkCreateDescriptorSetLayout(m_vkDevice, &layoutCreateInfo, nullptr, &m_uniformLayout);
	if (res != VK_SUCCESS)
	{
		return res;
	}

	return VK_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
VkResult VulkanRender::CreateDescriptorSetPool()
{
	VkResult res = VK_INCOMPLETE;

	VkDescriptorPoolSize poolSize = {};
		poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSize.descriptorCount = 1;

	VkDescriptorPoolCreateInfo poolCreateInfo = {};
		poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolCreateInfo.poolSizeCount = 1;
		poolCreateInfo.pPoolSizes = &poolSize;
		poolCreateInfo.maxSets = 1;

	res = vkCreateDescriptorPool(m_vkDevice, &poolCreateInfo, nullptr, &m_vkDescriptorPool);
	if (res != VK_SUCCESS)
	{
		return res;
	}

	VkDescriptorSetLayout layouts[] = { m_uniformLayout };
	VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_vkDescriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = layouts;

	res = vkAllocateDescriptorSets(m_vkDevice, &allocInfo, &m_vkDescriptorSet);
	if (res != VK_SUCCESS)
	{
		return res;
	}

	VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = m_MVPBuffer;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(ModelView);

	VkWriteDescriptorSet descriptorWrite = {};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = m_vkDescriptorSet;
		descriptorWrite.dstBinding = 0;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &bufferInfo;

	vkUpdateDescriptorSets(m_vkDevice, 1, &descriptorWrite, 0, nullptr);

	return VK_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
VkResult VulkanRender::UpdateMVP()
{
	VkResult res = VK_INCOMPLETE;

	rotationAngle += ROTATION_SPEED;

	//Update
	float rotationQuat[4] = { cos(rotationAngle / 2), 0, sin(rotationAngle / 2), 0 };
	float length = sqrt(rotationQuat[0] * rotationQuat[0] + rotationQuat[1] * rotationQuat[1] + rotationQuat[2] * rotationQuat[2] + rotationQuat[3] * rotationQuat[3]);
	rotationQuat[0] = rotationQuat[0] / length;
	rotationQuat[1] = rotationQuat[1] / length;
	rotationQuat[2] = rotationQuat[2] / length;
	rotationQuat[3] = rotationQuat[3] / length;

	m_MVP.model[0] = 1 - 2 * rotationQuat[2] * rotationQuat[2] - 2 * rotationQuat[3] * rotationQuat[3];
	m_MVP.model[1] = 2 * rotationQuat[1] * rotationQuat[2] - 2 * rotationQuat[0] * rotationQuat[3];
	m_MVP.model[2] = 2 * rotationQuat[1] * rotationQuat[3] + 2 * rotationQuat[0] * rotationQuat[2];
	m_MVP.model[3] = 0;

	m_MVP.model[4] = 2 * rotationQuat[1] * rotationQuat[2] + 2 * rotationQuat[0] * rotationQuat[3];
	m_MVP.model[5] = 1 - 2 * rotationQuat[1] * rotationQuat[1] - 2 * rotationQuat[3] * rotationQuat[3];
	m_MVP.model[6] = 2 * rotationQuat[2] * rotationQuat[3] - 2 * rotationQuat[0] * rotationQuat[1];
	m_MVP.model[7] = 0;

	m_MVP.model[8] = 2 * rotationQuat[1] * rotationQuat[3] - 2 * rotationQuat[0] * rotationQuat[2];
	m_MVP.model[9] = 2 * rotationQuat[2] * rotationQuat[3] + 2 * rotationQuat[0] * rotationQuat[1];
	m_MVP.model[10] = 1 - 2 * rotationQuat[1] * rotationQuat[1] - 2 * rotationQuat[2] * rotationQuat[2];
	m_MVP.model[11] = 0;

	m_MVP.model[12] = 0;
	m_MVP.model[13] = 0;
	m_MVP.model[14] = 0;
	m_MVP.model[15] = 1;

	void* bufferData = NULL;
	vkMapMemory(m_vkDevice, m_MVPMemory, 0, sizeof(ModelView), 0, &bufferData);
	memcpy(bufferData, &m_MVP, sizeof(ModelView));
	vkUnmapMemory(m_vkDevice, m_MVPMemory);

	return VK_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
VkResult VulkanRender::BuildMVP()
{
	// No rotation
	for (auto cell : m_MVP.model)
    {
		cell = 0.0f;
	}
	m_MVP.model[0] = 1;
	m_MVP.model[5] = 1;
	m_MVP.model[10] = 1;
	m_MVP.model[15] = 1;

	DirectX::XMMATRIX projection = DirectX::XMMatrixPerspectiveFovRH(3.14f / 2.0f, 0.75f, 0.01f, 1000.0f);
	DirectX::XMMATRIX view = DirectX::XMMatrixLookAtRH(DirectX::XMVectorSet(0.0f, -3.0f, 7.0f, 0.0f), DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f), DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
	for (int i = 0; i < 4; i++)
    {
		for (int j = 0; j < 4; j++)
        {
			m_MVP.proj[4 * i + j] = DirectX::XMVectorGetByIndex(projection.r[i],j);
			m_MVP.view[4 * i + j] = DirectX::XMVectorGetByIndex(view.r[i], j);
		}
	}

	return VK_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
VkInstance VulkanRender::GetVkInstance() const
{
    return m_vkInstance;
}

//-------------------------------------------------------------------------------------------------
VkDevice VulkanRender::GetVkDevice() const
{
    return m_vkDevice;
}

//-------------------------------------------------------------------------------------------------
VkSurfaceKHR VulkanRender::GetVkSurface() const
{
    return m_vkSurfaceKHR;
}

//-------------------------------------------------------------------------------------------------
VkQueue VulkanRender::GetVkQueue() const
{
    return m_presentQueue;
}

//-------------------------------------------------------------------------------------------------
uint32_t VulkanRender::GetVkQueueID() const
{
    return m_presentQueueIdx;
}

//-------------------------------------------------------------------------------------------------
int VulkanRender::GetCurrentSwapChainImageIndex() const
{
    return m_CurrentSwapChainImageIndex;
}

//-------------------------------------------------------------------------------------------------
VkImage VulkanRender::GetCurrentSwapChainImage() const
{
    if ((m_CurrentSwapChainImageIndex >= 0) && 
        (m_CurrentSwapChainImageIndex<m_swapChainImages.size()))
    {
        return m_swapChainImages[m_CurrentSwapChainImageIndex];
    }
    return VK_NULL_HANDLE;
}
