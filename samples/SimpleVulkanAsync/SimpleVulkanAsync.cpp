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

#include "SimpleVulkanAsync.h"
#include <DirectXMath.h>
#include <iostream>

static const std::vector<const char*> instanceExtensions = { "VK_KHR_surface", "VK_KHR_win32_surface", "VK_EXT_debug_report"};
static const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

//Math needed for cube and MVP, all angle measurements are in Radians
#define ROTATION_SPEED 0.0005f
#define IMAGE_HEIGHT 1080
#define IMAGE_WIDTH 1920

int _tmain(int argc, _TCHAR* argv[]) {
	VulkanAsync* sample = new VulkanAsync();

	do
	{
		sample->Draw();
		
		MSG msg = {};

		if (PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE))
		{
			if (msg.message == WM_KEYDOWN)
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
				break;
			}
			else if (msg.message == WM_QUIT)
			{
				break;
			}
		}
	} while (true);


	delete sample;

    return 0;
}


//For windows
LRESULT CALLBACK WndProc(HWND hwnd,
	UINT msg,
	WPARAM wParam,
	LPARAM lParam)
{
	switch (msg)
	{
		case WM_ACTIVATE:
		{
			bool active = LOWORD(wParam) != WA_INACTIVE;
			return 0;
		}
		break;
	}
	return DefWindowProc(hwnd,
		msg,
		wParam,
		lParam);
}

bool VulkanAsync::Init(){
    VkResult vkResult;
	
    vkResult = PreInitVulkan();
    if (VK_SUCCESS != vkResult)
    {
        std::cout << "Failed to pre-initialize Vulkan\n";
        return false;
    }

    ALVR_RESULT res;
    res = InitALVR();
    if (ALVR_OK != res)
    {
        std::cout << "Failed to initialize liquidvr\n";
        return false;
    }

    //perform surface to image
    res = ExecuteCompute();
    if (ALVR_OK != res)
    {
        std::cout << "Failed to execute compute\n";
        return false;
    }

    vkResult = PostInitVulkan();
    if (VK_SUCCESS != vkResult)
    {
        std::cout << "Failed to post-initialize Vulkan\n";
        return false;
    }

	return true;
}

ALVR_RESULT VulkanAsync::InitVKFnPtr(HMODULE vulkanDLL)
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
	vkDestroySampler = (PFN_vkDestroySampler)GetProcAddress(vulkanDLL, "vkDestroySampler");
	CHECK_RETURN(NULL != vkDestroySampler, ALVR_FAIL, "Failed to find vkDestroySampler");
	vkCmdBlitImage = (PFN_vkCmdBlitImage)GetProcAddress(vulkanDLL, "vkCmdBlitImage");
	CHECK_RETURN(NULL != vkCmdBlitImage, ALVR_FAIL, "Failed to find vkCmdBlitImage");

	return ALVR_OK;
}

ALVR_RESULT VulkanAsync::InitVKFnExtPtr()
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

VkResult VulkanAsync::PreInitVulkan(){ //TODO: change to check_return
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
	if (ALVR_OK != InitVKFnExtPtr())
	{
		return VK_INCOMPLETE;
	}
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

    m_imageHeight = IMAGE_HEIGHT;
    m_imageWidth = IMAGE_WIDTH;

    return VK_SUCCESS;
}

VkResult VulkanAsync::PostInitVulkan(){
    VkResult res = VK_INCOMPLETE;

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

VulkanAsync::VulkanAsync(){
	Init();
}

VulkanAsync::~VulkanAsync(){
	Shutdown();
}

bool VulkanAsync::Shutdown(){
	
	//Release all LVR
	m_ComputeContext.Release();
	m_ComputeTask.Release();
	m_computeFence.Release();
	m_GridConstantBuffer.Release();
	m_gridOutputSurface.Release();
	m_ComputeShader.clear();

	//Destroy everything in reverse order
	if (m_vkDevice != NULL)
	{
		if (m_textureSampler != NULL)
		{
			vkDestroySampler(m_vkDevice, m_textureSampler, nullptr);
		}
		if (m_textureView != NULL)
		{
			vkDestroyImageView(m_vkDevice, m_textureView, nullptr);
		}
		if (m_cubeTexture != NULL)
		{
			vkDestroyImage(m_vkDevice, m_cubeTexture, nullptr);
		}
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
		if (m_textureMemory!= NULL)
		{
			vkFreeMemory(m_vkDevice, m_textureMemory, nullptr);
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
		if (m_vkCommandPool != NULL)
		{
			vkDestroyCommandPool(m_vkDevice, m_vkCommandPool, nullptr);
		}
		for (auto frameBuffer : m_swapChainFrameBuffers){
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
		if (m_uniformLayout != NULL){
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

VkResult VulkanAsync::CreateInstance(){
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
	//check return on foundextensions
	if (foundExtensions < instanceExtensions.size()){
		return VK_INCOMPLETE;
	}

	VkInstanceCreateInfo creationInfo = {};
		creationInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        creationInfo.enabledExtensionCount = (uint32_t) instanceExtensions.size();
		creationInfo.pApplicationInfo = &appInfo;
		creationInfo.pNext = nullptr;
		creationInfo.ppEnabledExtensionNames = instanceExtensions.data();

	res = vkCreateInstance(&creationInfo, nullptr, &m_vkInstance);
	if (res != VK_SUCCESS)
	{
		return res;
	}

	return VK_SUCCESS;
}

VkResult VulkanAsync::CreateDevice(){
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

VkResult VulkanAsync::GetPhysicalDevice(){
	VkResult res = VK_INCOMPLETE;
	
	uint32_t deviceCount = 0;
	VkPhysicalDeviceProperties physicalDeviceProps;

	//Windows Specific
	HWND windowHandle = NULL;
	HINSTANCE hInstance = (HINSTANCE)GetModuleHandle(NULL);

	WNDCLASSEX wcex = {};
		wcex.cbSize = sizeof(WNDCLASSEX);
		wcex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_NOCLOSE;
		wcex.lpfnWndProc = WndProc;
		wcex.cbClsExtra = NULL;
		wcex.cbWndExtra = NULL;
		wcex.hInstance = hInstance;
		wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
		wcex.hIcon = NULL;
		wcex.lpszMenuName = NULL;
		wcex.lpszClassName = L"Vulkan Async";
		wcex.hIconSm = NULL;

	RegisterClassEx(&wcex);

	windowHandle = CreateWindowEx(NULL,
		L"Vulkan Async",
		L"Vulkan Async",
		WS_POPUP,
		CW_USEDEFAULT, CW_USEDEFAULT,
		800, 600,
		NULL,
		NULL,
		hInstance,
		NULL);

	if (!windowHandle)
	{
		return VK_INCOMPLETE;
	}

	ShowWindow(windowHandle, SW_NORMAL);
	UpdateWindow(windowHandle);

	//Create the windows surface
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

	//Check for suitable device
	for (auto device : devices)
	{
		VkPhysicalDeviceProperties deviceProps = {};
		vkGetPhysicalDeviceProperties(device, &deviceProps);

		uint32_t queueFamilycount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilycount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilycount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilycount, queueFamilyProperties.data());

		//Just take the first one that has the needed queues
		for (uint32_t i = 0; i < queueFamilycount; i++){
			VkBool32 presentSupport = false;
			res = vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_vkSurfaceKHR, &presentSupport);
			if (presentSupport && queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT | VK_QUEUE_COMPUTE_BIT)
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

VkResult VulkanAsync::CreateLogicalDevice(){ 
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
	for (auto deviceEx : deviceExtensions){
		bool extensionMissing = true;
		for (auto availEx : availableExtensions){
			if (strcmp(availEx.extensionName, deviceEx) == 0)
			{
				extensionMissing = false;
				break;
			}
		}
		if (extensionMissing){
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
		deviceCreateInfo.pEnabledFeatures = &deviceFeats;
		deviceCreateInfo.enabledExtensionCount = (uint32_t)deviceExtensions.size();
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

VkResult VulkanAsync::CreateSwapChain(){
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

	for (auto format : formats){
		if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			swapChainCreateInfo.imageFormat = format.format;
			swapChainCreateInfo.imageColorSpace = format.colorSpace;
			m_swapChainImageFormat = format.format;
			break;
		}
	}

	for (auto present : presentMode){
		if (present == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			swapChainCreateInfo.presentMode = present;
		}
	}

	//Create swap chain
	res = vkCreateSwapchainKHR(m_vkDevice, &swapChainCreateInfo, nullptr, &m_vkSwapChain);
	if (res != VK_SUCCESS)
	{
		return res;
	}

	//Store images in the chain
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
	for (uint32_t i = 0; i < m_swapChainImages.size(); i++){
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

VkResult VulkanAsync::CreatePipeline(){
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

	//Fixed Stages
	VkPipelineVertexInputStateCreateInfo vertInputInfo = {};
		vertInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertInputInfo.vertexBindingDescriptionCount = 1;
		vertInputInfo.pVertexBindingDescriptions = &m_vertexBindingDesc;
		vertInputInfo.vertexAttributeDescriptionCount = (uint32_t)m_vertexAttribDesc.size();
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


	//cleanup
	vkDestroyShaderModule(m_vkDevice, vertModule, nullptr);
	vkDestroyShaderModule(m_vkDevice, fragModule, nullptr);

	return VK_SUCCESS;
}

VkResult VulkanAsync::CreateRenderPass(){
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

VkResult VulkanAsync::CreateFrameBuffers(){
	VkResult res = VK_INCOMPLETE;

	m_swapChainFrameBuffers.resize(m_SwapChainImageViews.size());

	for (uint32_t i = 0; i < m_SwapChainImageViews.size(); i++){
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
VkResult VulkanAsync::CreateCommands(){
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

	//Command buffers
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

	for (uint32_t i = 0; i < m_vkCommandBuffers.size(); i++){
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

		vkCmdDrawIndexed(m_vkCommandBuffers[i], (uint32_t)m_indexes.size(), 1, 0, 0, 0);

		vkCmdEndRenderPass(m_vkCommandBuffers[i]);
		res = vkEndCommandBuffer(m_vkCommandBuffers[i]);
		if (res != VK_SUCCESS){
			return res;
		}
	}

	return VK_SUCCESS;
}

VkResult VulkanAsync::LoadShaders(){
	VkResult res = VK_INCOMPLETE;

    wchar_t path[5 * MAX_PATH];
    ::GetModuleFileNameW(NULL, path, _countof(path));
    std::wstring filepath(path);
    std::wstring::size_type slash = filepath.find_last_of(L"\\/");
    filepath = filepath.substr(0, slash + 1);
    std::wstring fileName = filepath + L"vert1.spv";
	//Load data from file
	std::ifstream vertFile(fileName, std::ios::ate | std::ios::binary);
    fileName = filepath + L"frag1.spv";
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

VkResult VulkanAsync::CreateSemaphores(){
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

bool VulkanAsync::Draw(){
	VkResult res = VK_INCOMPLETE;

	//Get Image from Swapchain-
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

	VkFence pipelineFence;
	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = 0;

	res = vkCreateFence(m_vkDevice, &fenceCreateInfo, nullptr, &pipelineFence);
	if (res != VK_SUCCESS)
	{
		return false;
	}

	res = vkQueueSubmit(m_presentQueue, 1, &submitInfo, pipelineFence);
	if (res != VK_SUCCESS)
	{
		return false;
	}
	res = vkWaitForFences(m_vkDevice, 1, &pipelineFence, VK_TRUE, UINT64_MAX);
	if (res != VK_SUCCESS)
	{
		return false;
	}

	//Present image
	VkSwapchainKHR swapChains[] = { m_vkSwapChain };
	VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;

	ALVRResourceVulkan resourceVulkan = {};
		m_gridOutputSurface->GetApiResource(ALVR_RENDER_API_VULKAN, &resourceVulkan);

	vkQueuePresentKHR(m_presentQueue, &presentInfo);
	vkDeviceWaitIdle(m_vkDevice);
	UpdateMVP();

	return true;
}

VkResult VulkanAsync::CreatePipelineInput(){
	VkResult res = VK_INCOMPLETE;

	m_vertexBindingDesc = {};
	m_vertexBindingDesc.binding = 0;
	m_vertexBindingDesc.stride = sizeof(Vertex);
	m_vertexBindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	m_vertexAttribDesc.resize(2); //One for each attribute in the vertex struct; Pos/UV

	//Position
	m_vertexAttribDesc[0].binding = 0;
	m_vertexAttribDesc[0].location = 0;
	m_vertexAttribDesc[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	m_vertexAttribDesc[0].offset = offsetof(Vertex, pos);

	//UV
	m_vertexAttribDesc[1].binding = 0;
	m_vertexAttribDesc[1].location = 1;
	m_vertexAttribDesc[1].format = VK_FORMAT_R32G32_SFLOAT;
	m_vertexAttribDesc[1].offset = offsetof(Vertex, uv);

	return VK_SUCCESS;
}

VkResult VulkanAsync::CreateVertexBuffers(){
	VkResult res = VK_INCOMPLETE;

	//Vertex data
	m_vertexes = {
		//Front
		{ { 1, 1, 1 }, { 1, 1 } }, //right Bottom
		{ { 1, -1, 1 }, { 1, 0 } }, //right top
		{ { -1, 1, 1 }, { 0, 1} }, //left Bottom
		{ { -1, -1, 1 }, { 0, 0 } }, //left Top
		//Right
		{ { 1, 1, -1 }, { 1, 1 } }, //back bottom
		{ { 1, -1, -1 }, { 1, 0 } }, //back top
		{ { 1, 1, 1 }, { 0, 1 } }, //front bottom
		{ { 1, -1, 1 }, { 0, 0 } }, //front top
		//left
		{ { -1, 1, 1 }, { 1, 1 } }, //front bottom
		{ { -1, -1, 1 }, { 1, 0 } }, //front top
		{ { -1, 1, -1 }, { 0, 1 } }, //back bottom
		{ { -1, -1, -1 }, { 0, 0 } }, //back top
		//Back
		{ { 1, 1, -1 }, { 0, 1 } }, //right Bottom
		{ { 1, -1, -1 }, { 0, 0 } }, //right top
		{ { -1, 1, -1 }, { 1, 1 } }, //left Bottom
		{ { -1, -1, -1 }, { 1, 0 } }, //left Top
		//top
		{ { 1, -1, 1 }, { 1, 0 } }, //front right
		{ { 1, -1, -1 }, { 1, 1 } }, //back right
		{ { -1, -1, 1 }, { 0, 0 } }, //front left
		{ { -1, -1, -1 }, { 0, 1 } }, //back left
		//bottom
		{ { 1, 1, -1 }, { 1, 0 } }, //right back
		{ { 1, 1, 1 }, { 1, 1 } }, //right front
		{ { -1, 1, -1 }, { 0, 0 } }, //left back
		{ { -1, 1, 1 }, { 0, 1 } } //left front
	};

	//Index data
	m_indexes = {
		//front
		0, 1, 2,
		3, 2, 1,
		//right
		4, 5, 6,
		7, 6, 5,
		//left
		8,  9,  10,
		11, 10, 9,
		//back
		14,13,12,
		13,14,15,
		//top
		16,17,18,
		19,18,17,
		//bottom
		20,21,22,
		23,22,21
	};

	BuildMVP();

	MakeBuffer(sizeof(Vertex) * m_vertexes.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_vertexBuffer, m_vertexMemory);
	MakeBuffer(sizeof(uint32_t) * m_indexes.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_indexBuffer, m_indexMemory);
	MakeBuffer(sizeof(ModelView), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_MVPBuffer, m_MVPMemory);
	MakeImage(m_imageWidth,m_imageHeight, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_cubeTexture, m_textureMemory);

    if (ALVR_OK != CopySurfaceToImage())
    {
        std::cout << "Failed to copy surface image\n";
        return VK_INCOMPLETE;
    }

	//set image view for cube texture
	VkImageViewCreateInfo viewCreateInfo = {};
		viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewCreateInfo.image = m_cubeTexture;
		viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;

		viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	
		viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewCreateInfo.subresourceRange.baseMipLevel = 0;
		viewCreateInfo.subresourceRange.levelCount = 1;
		viewCreateInfo.subresourceRange.baseArrayLayer = 0;
		viewCreateInfo.subresourceRange.layerCount = 1;

	res = vkCreateImageView(m_vkDevice, &viewCreateInfo, nullptr, &m_textureView);
	if (res != VK_SUCCESS)
	{
		return res;
	}

	//texture sampler
	VkSamplerCreateInfo samplerCreateInfo = {};
		samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

		samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;

		samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerCreateInfo.mipLodBias = 0;
		samplerCreateInfo.minLod = 0;
		samplerCreateInfo.maxLod = 0;

		samplerCreateInfo.anisotropyEnable = VK_TRUE;
		samplerCreateInfo.maxAnisotropy = 16;

		samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
		samplerCreateInfo.compareEnable = VK_FALSE;
		samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	res = vkCreateSampler(m_vkDevice, &samplerCreateInfo, nullptr, &m_textureSampler);
	if (res != VK_SUCCESS)
	{
		return res;
	}

	//Copy data into buffers

	//vertex
	void* bufferData = NULL;
	res = vkMapMemory(m_vkDevice, m_vertexMemory, 0, sizeof(Vertex) * m_vertexes.size(), 0, &bufferData);
	if (res != VK_SUCCESS || bufferData == NULL)
	{
		return res;
	}
	memcpy(bufferData, m_vertexes.data(), (size_t)sizeof(Vertex) * m_vertexes.size());
	vkUnmapMemory(m_vkDevice, m_vertexMemory);
	bufferData = NULL;

	//index
	res = vkMapMemory(m_vkDevice, m_indexMemory, 0, sizeof(uint32_t) * m_indexes.size(), 0, &bufferData);
	if (res != VK_SUCCESS || bufferData == NULL)
	{
		return res;
	}
	memcpy(bufferData, m_indexes.data(), (size_t)sizeof(uint32_t) * m_indexes.size());
	vkUnmapMemory(m_vkDevice, m_indexMemory);
	bufferData = NULL;

	//mvp
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

VkResult VulkanAsync::MakeBuffer(VkDeviceSize size, VkBufferUsageFlags use, VkMemoryPropertyFlags props, VkBuffer& buffer, VkDeviceMemory& memory){
	VkResult res = VK_INCOMPLETE;
	
	VkBufferCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		createInfo.size = size;
		createInfo.usage = use;
		createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	res = vkCreateBuffer(m_vkDevice, &createInfo, nullptr, &buffer);
	if (res != VK_SUCCESS){
		return res;
	}

	VkMemoryRequirements memReqs = {};
	vkGetBufferMemoryRequirements(m_vkDevice, buffer, &memReqs);

	VkPhysicalDeviceMemoryProperties memProps;
	vkGetPhysicalDeviceMemoryProperties(m_vkPhysicalDevice, &memProps);

	uint32_t memType = -1;
	for (uint32_t i = 0; i < memProps.memoryTypeCount; i++){
		if (memProps.memoryTypes[i].propertyFlags & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)){

			if ((memProps.memoryTypes[i].propertyFlags & memReqs.memoryTypeBits))
			{
				memType = i;
				break;
			}
		}
	}

	if (memType == ((uint32_t)-1)){
		return VK_ERROR_DEVICE_LOST;
	}

	VkMemoryAllocateInfo allocInfo = {};
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

VkResult VulkanAsync::MakeImage(uint32_t imageWidth, uint32_t imageHeight, VkFormat format, VkImageTiling tiling, VkImageUsageFlags use, VkMemoryPropertyFlags props, VkImage& image, VkDeviceMemory& memory){
	VkResult res = VK_INCOMPLETE;

	VkImageCreateInfo imageCreateInfo = {};
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.format = format;
		imageCreateInfo.tiling = tiling;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageCreateInfo.usage =  VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		imageCreateInfo.extent.width = imageWidth;
		imageCreateInfo.extent.height = imageHeight;
		imageCreateInfo.extent.depth = 1;
		imageCreateInfo.mipLevels = 1;
		imageCreateInfo.arrayLayers = 1;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;

	res = vkCreateImage(m_vkDevice, &imageCreateInfo, nullptr, &image);
	if (res != VK_SUCCESS)
	{
		return res;
	}

	VkMemoryRequirements memReqs = {};
	vkGetImageMemoryRequirements(m_vkDevice, image, &memReqs);

	VkPhysicalDeviceMemoryProperties memProps;
	vkGetPhysicalDeviceMemoryProperties(m_vkPhysicalDevice, &memProps);

	uint32_t memType = -1;
	for (uint32_t i = 0; i < memProps.memoryTypeCount; i++){
		if (memProps.memoryTypes[i].propertyFlags & (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)){

			if ((memProps.memoryTypes[i].propertyFlags & memReqs.memoryTypeBits))
			{
				memType = i;
				break;
			}
		}
	}

	VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memReqs.size;
		allocInfo.memoryTypeIndex = memType;

	res = vkAllocateMemory(m_vkDevice, &allocInfo, nullptr, &memory);
	if (res != VK_SUCCESS)
	{
		return res;
	}
	res = vkBindImageMemory(m_vkDevice, image, memory, 0);
	if (res != VK_SUCCESS)
	{
		return res;
	}

	return VK_SUCCESS;
}

VkResult VulkanAsync::CreateDescriptorSetLayout(){
	VkResult res = VK_INCOMPLETE;

	VkDescriptorSetLayoutBinding MVPLayoutBinding = {};
		MVPLayoutBinding.binding = 0;
		MVPLayoutBinding.descriptorCount = 1;
		MVPLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		MVPLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
		samplerLayoutBinding.binding = 1;
		samplerLayoutBinding.descriptorCount = 1;
		samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		samplerLayoutBinding.pImmutableSamplers = nullptr;

	std::vector<VkDescriptorSetLayoutBinding> bindings = { MVPLayoutBinding, samplerLayoutBinding };
	VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
		layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutCreateInfo.bindingCount = (uint32_t)bindings.size();
		layoutCreateInfo.pBindings = bindings.data();

	res = vkCreateDescriptorSetLayout(m_vkDevice, &layoutCreateInfo, nullptr, &m_uniformLayout);
	if (res != VK_SUCCESS)
	{
		return res;
	}

	return VK_SUCCESS;
}

VkResult VulkanAsync::CreateDescriptorSetPool(){
	VkResult res = VK_INCOMPLETE;

	VkDescriptorPoolSize poolSize = {};
		poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSize.descriptorCount = 1;

	VkDescriptorPoolSize samplerSize = {};
		samplerSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerSize.descriptorCount = 1;

	std::vector<VkDescriptorPoolSize> sizes = { poolSize, samplerSize };
	VkDescriptorPoolCreateInfo poolCreateInfo = {};
		poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolCreateInfo.poolSizeCount = (uint32_t)sizes.size();
		poolCreateInfo.pPoolSizes = sizes.data();
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
	VkDescriptorImageInfo imageInfo = {};
		imageInfo.sampler = m_textureSampler;
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = m_textureView;

	VkWriteDescriptorSet descriptorWrite = {};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = m_vkDescriptorSet;
		descriptorWrite.dstBinding = 0;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &bufferInfo;

	VkWriteDescriptorSet descriptorSampler = {};
		descriptorSampler.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorSampler.dstSet = m_vkDescriptorSet;
		descriptorSampler.dstBinding = 1;
		descriptorSampler.dstArrayElement = 0;
		descriptorSampler.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorSampler.descriptorCount = 1;
		descriptorSampler.pImageInfo = &imageInfo;

	std::vector<VkWriteDescriptorSet> descriptors = { descriptorWrite, descriptorSampler };

	vkUpdateDescriptorSets(m_vkDevice, (uint32_t)descriptors.size(), descriptors.data(), 0, nullptr);

	return VK_SUCCESS;
}

VkResult VulkanAsync::UpdateMVP(){
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

VkResult VulkanAsync::BuildMVP(){

	//No rotation
	for (auto cell : m_MVP.model){
		cell = 0.0f;
	}
	m_MVP.model[0] = 1;
	m_MVP.model[5] = 1;
	m_MVP.model[10] = 1;
	m_MVP.model[15] = 1;

	//Build projection and view matrix
	DirectX::XMMATRIX projection = DirectX::XMMatrixPerspectiveFovRH(3.14f / 2.0f, 0.75f, 0.01f, 1000.0f);
	DirectX::XMMATRIX view = DirectX::XMMatrixLookAtRH(DirectX::XMVectorSet(0.0f, -3.0f, 4.0f, 0.0f), DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f), DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
	for (int i = 0; i < 4; i++){
		for (int j = 0; j < 4; j++){
			m_MVP.proj[4 * i + j] = DirectX::XMVectorGetByIndex(projection.r[i],j);
			m_MVP.view[4 * i + j] = DirectX::XMVectorGetByIndex(view.r[i], j);
		}
	}

	return VK_SUCCESS;
}

VkCommandBuffer	VulkanAsync::BeginOneTimeCommand(){
	VkCommandBuffer tempCommandBuffer;

	VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = m_vkCommandPool;
		allocInfo.commandBufferCount = 1;

	vkAllocateCommandBuffers(m_vkDevice, &allocInfo, &tempCommandBuffer);

	VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(tempCommandBuffer, &beginInfo);

	return tempCommandBuffer;
}

void VulkanAsync::EndOneTimeCommand(VkCommandBuffer tempCommandBuffer){
	vkEndCommandBuffer(tempCommandBuffer);
	VkSubmitInfo commandSubmitInfo = {};
		commandSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		commandSubmitInfo.commandBufferCount = 1;
		commandSubmitInfo.pCommandBuffers = &tempCommandBuffer;
		commandSubmitInfo.pSignalSemaphores = nullptr;
		commandSubmitInfo.signalSemaphoreCount = 0;
		commandSubmitInfo.waitSemaphoreCount = 0;
		commandSubmitInfo.pWaitDstStageMask = nullptr;
		commandSubmitInfo.pNext = nullptr;

	vkQueueSubmit(m_presentQueue, 1, &commandSubmitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(m_presentQueue);

	vkFreeCommandBuffers(m_vkDevice, m_vkCommandPool, 1, &tempCommandBuffer);
}

ALVR_RESULT VulkanAsync::InitALVR(){
	ALVR_RESULT res = ALVR_OK;

    m_hALVRDLL = LoadLibraryW(ALVR_DLL_NAME);
    CHECK_RETURN(NULL != m_hALVRDLL, ALVR_FAIL, "Failed to load liquidvr dll");

	ALVRInit_Fn pInit = (ALVRInit_Fn)GetProcAddress(m_hALVRDLL, ALVR_INIT_FUNCTION_NAME);
	res = pInit(ALVR_FULL_VERSION, (void**)&m_ALVRFactory);
	CHECK_ALVR_ERROR_RETURN(res, ALVR_INIT_FUNCTION_NAME << L"failed");

#if defined(AFFINITY_WORK_AROUND)

	res = m_ALVRFactory->CreateGpuAffinity(&m_pLvrAffinity);
	CHECK_ALVR_ERROR_RETURN(res, L"CreateGpuAffinity() failed");

	res = m_pLvrAffinity->EnableGpuAffinity(ALVR_GPU_AFFINITY_FLAGS_NONE);
	CHECK_ALVR_ERROR_RETURN(res, L"EnableGpuAffinity() failed");

	res = res = m_pLvrAffinity->DisableGpuAffinity();
	CHECK_ALVR_ERROR_RETURN(res, L"DisableGpuAffinity() failed");

#endif

	//create alvr vulkan device ex
	res = m_ALVRFactory->CreateALVRExtensionVulkan(m_vkInstance, NULL, &m_VulkanEx);
	CHECK_ALVR_ERROR_RETURN(res, L"CreateALVRExtensionVulkan() failed");

	res = m_VulkanEx->CreateALVRDeviceExVulkan(m_vkDevice,NULL ,&m_deviceExVulkan );

	//Setting up the async compute
	ALVRComputeContextDesc computeDesc = {};
	computeDesc.flags = ALVR_COMPUTE_NONE;
    res = m_ALVRFactory->CreateComputeContext(m_deviceExVulkan, 0, &computeDesc, &m_ComputeContext);
    CHECK_ALVR_ERROR_RETURN(res, L"CreateComputeContext() failed");

	GridParameters gridParams = {};
		gridParams.colorBg = DirectX::XMFLOAT4(1, 0, 0, 1); //DirectX::Colors::Red
		gridParams.colorFg = DirectX::XMFLOAT4(0.83f, 0.83f, 0.83f, 1.0f); //DirectX::Colors::LightGray
		gridParams.width = m_imageWidth;
		gridParams.height = m_imageHeight;
		gridParams.gridWidth = 100;
		gridParams.lineWidth = 10;

	ALVRBufferDesc bufferDesc = { 0 };
		bufferDesc.apiSupport = ALVR_RESOURCE_API_ASYNC_COMPUTE | ALVR_RESOURCE_API_VULKAN;
		bufferDesc.bufferFlags = ALVR_BUFFER_CONSTANT;
		bufferDesc.cpuAccessFlags = ALVR_CPU_ACCESS_WRITE;
		bufferDesc.structureStride = 0;
		bufferDesc.format = ALVR_FORMAT_UNKNOWN;
        bufferDesc.size = sizeof(GridParameters);

    // Read grid shader
    wchar_t path[5 * MAX_PATH];
    ::GetModuleFileNameW(NULL, path, _countof(path));
    std::wstring filepath(path);
    std::wstring::size_type slash = filepath.find_last_of(L"\\/");
    filepath = filepath.substr(0, slash + 1);
    std::wstring fileName = filepath + L"GridShader.cso";

    std::ifstream computeFile(fileName, std::ios::ate | std::ios::binary);
    if (!computeFile.is_open())
    {
        return res;
    }
    size_t computeFileSize = (size_t)computeFile.tellg();
    m_ComputeShader.resize(computeFileSize);
    computeFile.seekg(0);
    computeFile.read(m_ComputeShader.data(), computeFileSize);
	computeFile.close();

	res = m_ComputeContext->CreateComputeTask(ALVR_SHADER_MODEL_D3D11, 0, m_ComputeShader.data(), m_ComputeShader.size(), &m_ComputeTask);
	CHECK_ALVR_ERROR_RETURN(res, L"CreateComputeTask(Grid) failed");

	ALVRVariantStruct bVariantStruct;
	bVariantStruct.type = ALVR_VARIANT_BOOL;
	bVariantStruct.boolValue = false;
	m_ComputeTask->SetProperty(ALVR_COMPUTE_PROPERTY_VALIDATE_RESOURCE_BINDING, bVariantStruct);

	res = m_ComputeContext->CreateBuffer(&bufferDesc, &m_GridConstantBuffer);
	CHECK_ALVR_ERROR_RETURN(res, L"CreateBuffer(Grid) failed");

	res = m_ComputeTask->BindConstantBuffer(0, m_GridConstantBuffer);
	CHECK_ALVR_ERROR_RETURN(res, L"BindConstantBuffer(Grid) failed");

	void* bufferData;
	res = m_GridConstantBuffer->Map(&bufferData);
	CHECK_ALVR_ERROR_RETURN(res, L"Map(Grid) failed");
	memcpy(bufferData, &gridParams, sizeof(gridParams));
	res = m_GridConstantBuffer->Unmap();
	CHECK_ALVR_ERROR_RETURN(res, L"Unmap(Grid) failed");

	//compute shader output
	ALVRSurfaceDesc surfaceDesc = {};
		surfaceDesc.type = ALVR_SURFACE_2D;
        surfaceDesc.surfaceFlags = ALVR_SURFACE_SHADER_OUTPUT | ALVR_SURFACE_SHADER_INPUT;
        surfaceDesc.apiSupport = ALVR_RESOURCE_API_ASYNC_COMPUTE | ALVR_RESOURCE_API_VULKAN;
		surfaceDesc.width = m_imageWidth;
		surfaceDesc.height = m_imageHeight;
		surfaceDesc.depth = 1;
		surfaceDesc.format = ALVR_FORMAT_R8G8B8A8_UNORM;
		surfaceDesc.sliceCount = 1;
		surfaceDesc.mipCount = 1;

	res = m_ComputeContext->CreateSurface(&surfaceDesc, &m_gridOutputSurface);
	CHECK_ALVR_ERROR_RETURN(res, L"CreateSurface(Grid) failed");

	res = m_ComputeTask->BindOutput(0, m_gridOutputSurface);
	CHECK_ALVR_ERROR_RETURN(res, L"BindOutput(Grid) failed");

	return ALVR_OK;
}

ALVR_RESULT VulkanAsync::ExecuteCompute(){
	ALVR_RESULT res = ALVR_OK;

	int THREAD_GROUP_SIZE = 8;
	ALVRPoint3D offset = { 0, 0, 0 };
	ALVRSize3D size = { (m_imageWidth + THREAD_GROUP_SIZE - 1) / THREAD_GROUP_SIZE, (m_imageHeight + THREAD_GROUP_SIZE - 1) / THREAD_GROUP_SIZE, 1 };

	res = m_ComputeContext->QueueTask(m_ComputeTask, &offset, &size);
	CHECK_ALVR_ERROR_RETURN(res, L"QueueTask() failed");

	m_ComputeContext->CreateFence(&m_computeFence);
	m_ComputeContext->Flush(m_computeFence);
	m_computeFence->Wait(2000);

	return ALVR_OK;
}

ALVR_RESULT VulkanAsync::CopySurfaceToImage(){
	ALVR_RESULT res = ALVR_OK;

	//Acquire the liquidVR surface as a vulkan image
	ALVRResourceVulkan resourceVulkan = {};
	res = m_gridOutputSurface->GetApiResource(ALVR_RENDER_API_VULKAN, &resourceVulkan);
	CHECK_ALVR_ERROR_RETURN(res, "GetApiResource() failed");
	CHECK_RETURN(ALVR_VULKAN_RESOURCE_IMAGE == resourceVulkan.resourceType, ALVR_FAIL, "Not a vulkan image");
	CHECK_RETURN(NULL != resourceVulkan.image, ALVR_FAIL, "resourceVulkan.image is null");

	//Setup the information to copy the image
	VkImageSubresourceLayers subResource = {};
		subResource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subResource.baseArrayLayer = 0;
		subResource.mipLevel = 0;
		subResource.layerCount = 1;

	VkImageCopy copyRegion = {};
		copyRegion.srcSubresource = subResource;
		copyRegion.dstSubresource = subResource;
		copyRegion.srcOffset = { 0, 0, 0 };
		copyRegion.dstOffset = { 0, 0, 0 };
		copyRegion.extent.height = m_imageHeight;
		copyRegion.extent.width = m_imageWidth;
		copyRegion.extent.depth = 1;

	//Transition the image layout to accept the transfer, execute transfer, transition back to drawing layout
    VkImageMemoryBarrier transitionBarrier = {};
        transitionBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        transitionBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        transitionBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        transitionBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        transitionBarrier.subresourceRange.baseArrayLayer = 0;
        transitionBarrier.subresourceRange.baseMipLevel = 0;
        transitionBarrier.subresourceRange.layerCount = 1;
        transitionBarrier.subresourceRange.levelCount = 1;

    VkCommandBuffer transitionCommandBuffer = BeginOneTimeCommand();
        transitionBarrier.image = m_cubeTexture;
        transitionBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        transitionBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        transitionBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        transitionBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        vkCmdPipelineBarrier(transitionCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &transitionBarrier);
    EndOneTimeCommand(transitionCommandBuffer);

    VkCommandBuffer tempCommandBuffer = BeginOneTimeCommand();
		vkCmdCopyImage(tempCommandBuffer, resourceVulkan.image, VK_IMAGE_LAYOUT_GENERAL, m_cubeTexture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
    EndOneTimeCommand(tempCommandBuffer);

    transitionCommandBuffer = BeginOneTimeCommand();
        transitionBarrier.image = m_cubeTexture;
        transitionBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        transitionBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        transitionBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        transitionBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        vkCmdPipelineBarrier(transitionCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &transitionBarrier);
    EndOneTimeCommand(transitionCommandBuffer);

	return ALVR_OK;
}
