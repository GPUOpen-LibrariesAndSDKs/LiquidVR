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

#include <conio.h>

#include "../../inc/LiquidVR.h"
#include "../common/inc/D3DHelper.h"
#include "../common/inc/LvrLogger.h"

#define VULKAN_FUNCTIONS
#include "VulkanRender.h"

// This example demonstrates the following:
// - Allocate surfaces in DX11
// - Open in Vulkan using LiquidVR
// - Render in Vulkan
// - Present in DX11
//
// There are two types of rendering in Vulkan:
// 1) Render of geometry
// 2) Render of images(simple image swapping)
// The third type of rendering is DX11 and does 
// not include Vulkan.  Pressing the 'r' key will
// iterate through the types of rendering

using namespace DirectX;

// Enable this define for drivers before 15.30 Crimson 
#define AFFINITY_WORK_AROUND // remove all under this define when GXX fixes issue related to LvrResources  + MGPU

namespace
{
    // TestImage 
    //
    class TestImage
    {
    public:
        TestImage(unsigned width, unsigned height, unsigned pixelSize,
            BYTE r, BYTE g, BYTE b, BYTE a, bool generateNoise = true )
            : m_buffer(NULL)
        {
            m_size = width * height * pixelSize;
            m_buffer = new char[m_size];
            //
            for (unsigned i = 0; i < m_size; i += pixelSize)
            {
                if (generateNoise && ((rand() % 100)>80))
                {
                    m_buffer[i] = 0;
                }
                else
                {
                    m_buffer[i] = r;
                }
                m_buffer[i + 1] = g;
                m_buffer[i + 2] = b;
                m_buffer[i + 3] = a;
            }
        }

        TestImage(const TestImage& rhs)
            : m_buffer(NULL), m_size(rhs.m_size)
        {
            m_buffer = new char[m_size];
            memcpy(m_buffer, rhs.m_buffer, m_size);
        }

        ~TestImage()
        {
            delete[] m_buffer;
        }

        void* GetImageBuffer() const
        {
            return m_buffer;
        }

        unsigned GetSize() const
        {
            return m_size;
        }

        bool Equals(const char *buffer) const
        {
            bool result = true;
            for (unsigned i = 0; i < m_size; i++)
            {
                char a = m_buffer[i];
                char b = buffer[i];
                if (a != b)
                {
                    result = false;
                    break;
                }
            }
            return result;
        }

    private:
        TestImage& operator=(const TestImage& rhs);

        char*       m_buffer;
        unsigned    m_size;
    };

    // Circular vector
    template <typename Type>
    class CircularVector : public std::vector<Type>
    {
    public:
        CircularVector() : std::vector<Type>(), index(0) {}

        Type& current()
        {
            return at(index);
        }

        const Type& current() const
        {
            return at(index);
        }

        void next()
        {
            index++;
            if (index == size())
                index = 0;
        }

    private:
        size_t index;
    };
}

//-------------------------------------------------------------------------------------------------
ALVR_RESULT Init();
ALVR_RESULT Terminate();
ALVR_RESULT SetupRenderTarget();
ALVR_RESULT Render();
ALVR_RESULT ClearFrame(bool bLeft);
ALVR_RESULT HandleKeyboard(UINT_PTR ch);

ALVR_RESULT ChangeDrawRepeat(bool bUp);

//-------------------------------------------------------------------------------------------------
// enums
//-------------------------------------------------------------------------------------------------

enum RenderType
{
    RenderDX11,
    RenderVulkanImage,
    RenderVulkanGeometry
};

//-------------------------------------------------------------------------------------------------
// options
//-------------------------------------------------------------------------------------------------
static const int                                g_iBackbufferCount = 2;
static volatile int                             g_iDrawRepeat = 2000; // use for GPU load on DX11 render path
static volatile bool                            g_bRotate = true;
static RenderType                               g_RenderType = RenderVulkanImage;

//-------------------------------------------------------------------------------------------------
static D3DHelper                                g_D3DHelper;
static HWND                                     g_hWindow = NULL;

static ALVRFactory*                             g_pFactory;
static ATL::CComPtr<ALVRDeviceExD3D11>          g_pLvrDevice;

#if defined(AFFINITY_WORK_AROUND)
static ATL::CComPtr<ALVRGpuAffinity>            m_pLvrAffinity;
#endif

static ATL::CComPtr<ALVRFence>                  g_pFenceD3D11;

HMODULE                                         g_hLiquidVRDLL = NULL;

static VulkanRender*                            g_pVulkanRender = NULL;
static VkImage                                  g_VkBackBuffer = {};

static CircularVector<TestImage>                g_Images;

static ATL::CComPtr<ALVRExtensionVulkan>        g_pExtensionVk; 
static ATL::CComPtr<ALVRDeviceExVulkan>         g_pDeviceEx;
static ATL::CComPtr<ID3D11Texture2D>            g_pBackBufferSrc;

static unsigned                                 g_Width = 0;
static unsigned                                 g_Height = 0;
static unsigned                                 g_PixelSize = 0;

//-------------------------------------------------------------------------------------------------
int _tmain(int argc, _TCHAR* argv[])
{
    ALVR_RESULT res = ALVR_OK;

    printf("Press 'r' to toggle  DX11 or Vulkan rendering.\nType any other key to exit:\n");

    res = Init();
    if(res != ALVR_OK)
    {
        Terminate();
        return -1;
    }

    MSG msg={0};
    while(true)
    {
        
        if(_kbhit())
        {
            char ch = _getch();
            
            if(ch == 0 || ch == (char)0xE0)
            {
                ch = _getch();
                if(ch == (char)0x48)
                {
                    ch = VK_UP;
                }
                else if(ch == (char)0x50)
                {
                    ch = VK_DOWN;
                }
            }
            if(HandleKeyboard(ch) != ALVR_OK)
            {            
                    break;
            }
        }
        
        if(PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
        {
            GetMessage(&msg, NULL, 0, 0);
            if(msg.message == WM_KEYDOWN)
            {
                if(HandleKeyboard((char)msg.wParam) != ALVR_OK)
                {
                    break;
                }
            }

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            // Set the render target, clear and then render
            res = SetupRenderTarget();
            if (res != ALVR_OK)
            {
                Terminate();
                return -1;
            }
            res = ClearFrame(true);
            if (res != ALVR_OK)
            {
                Terminate();
                return -1;
            }
            res = Render();
            if (res != ALVR_OK)
            {
                Terminate();
                return -1;
            }

            // Present the frame
            res = g_D3DHelper.Present();
            
            // Post work
            if ((RenderDX11 == g_RenderType) && g_bRotate)
            {
                g_D3DHelper.Animate();
            }
            else if (RenderVulkanImage == g_RenderType)
            {
                // Slow down if we are showing the image flipping
                Sleep(2000);
            }
        }

    }

    Terminate();

    return 0;
}

//-------------------------------------------------------------------------------------------------
LRESULT CALLBACK MyDefWindowProcW(
    __in HWND hWnd,
    __in UINT Msg,
    __in WPARAM wParam,
    __in LPARAM lParam)
{
    switch(Msg)
    {
        case WM_ACTIVATE:
        {
            bool active = LOWORD(wParam) != WA_INACTIVE;
            return 0;
        }
        break;
    }
    return DefWindowProc(hWnd, Msg, wParam, lParam);
}

//-------------------------------------------------------------------------------------------------
static RECT g_MonitorWorkArea;

//-------------------------------------------------------------------------------------------------
BOOL CALLBACK MyDisplayEnumProc(
    _In_  HMONITOR hMonitor,
    _In_  HDC hdcMonitor,
    _In_  LPRECT lprcMonitor,
    _In_  LPARAM dwData
    )
{
    WCHAR *name = (WCHAR *)dwData;

    MONITORINFOEX mi;
    mi.cbSize = sizeof(mi);
    GetMonitorInfo(hMonitor, &mi);
    if(wcscmp(name, mi.szDevice) == 0)
    {
        g_MonitorWorkArea = mi.rcWork;
        return FALSE;
    }
    return TRUE;
}

//-------------------------------------------------------------------------------------------------
const char* CurrentRenderString()
{
    switch (g_RenderType)
    {
    case RenderDX11: return           "Allocate surface in DX, render in DX, present in DX              ";
    case RenderVulkanImage: return    "Allocate surfaces in DX, render images in Vulkan, present in DX  ";
    case RenderVulkanGeometry: return "Allocate surfaces in DX, render geometry in Vulkan, present in DX";
    }
    return "";
}

//-------------------------------------------------------------------------------------------------
ALVR_RESULT Init()
{
    HRESULT hr = S_OK;
    ALVR_RESULT res = ALVR_OK;

    //---------------------------------------------------------------------------------------------
    // Init ALVR
    //---------------------------------------------------------------------------------------------

    g_hLiquidVRDLL = LoadLibraryW(ALVR_DLL_NAME);
    CHECK_RETURN(g_hLiquidVRDLL != NULL, ALVR_FAIL, L"DLL " << ALVR_DLL_NAME << L" is not found");

    ALVRInit_Fn pInit = (ALVRInit_Fn)GetProcAddress(g_hLiquidVRDLL, ALVR_INIT_FUNCTION_NAME);
    res = pInit(ALVR_FULL_VERSION, (void**)&g_pFactory);
    CHECK_ALVR_ERROR_RETURN(res, ALVR_INIT_FUNCTION_NAME << L"failed");

#if defined(AFFINITY_WORK_AROUND)

    res = g_pFactory->CreateGpuAffinity(&m_pLvrAffinity);
    //    CHECK_ALVR_ERROR_RETURN(res, L"CreateGpuAffinity() failed");

    if(m_pLvrAffinity != NULL)
    {
        res = m_pLvrAffinity->EnableGpuAffinity(ALVR_GPU_AFFINITY_FLAGS_NONE);
        CHECK_ALVR_ERROR_RETURN(res, L"EnableGpuAffinity() failed");

        res = m_pLvrAffinity->DisableGpuAffinity();
        CHECK_ALVR_ERROR_RETURN(res, L"EnableGpuAffinity() failed");
    }
#endif

    //---------------------------------------------------------------------------------------------
    // create D3D11 device
    //---------------------------------------------------------------------------------------------

    res = g_D3DHelper.CreateD3D11Device();
    CHECK_ALVR_ERROR_RETURN(res, L"CreateD3D11Device() failed");

    res = g_pFactory->CreateALVRDeviceExD3D11(g_D3DHelper.m_pd3dDevice, NULL, &g_pLvrDevice);
    CHECK_ALVR_ERROR_RETURN(res, L"CreateALVRDeviceExD3D11() failed");

    res = g_pLvrDevice->CreateFence(&g_pFenceD3D11);
    CHECK_ALVR_ERROR_RETURN(res, L"CreateFence() failed");

    //---------------------------------------------------------------------------------------------
    // Create window
    //---------------------------------------------------------------------------------------------

    HINSTANCE hInstance = (HINSTANCE)GetModuleHandle(NULL);

    WNDCLASSEX wcex     ={0};
    wcex.cbSize         = sizeof(WNDCLASSEX);
    wcex.style          = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wcex.lpfnWndProc    = MyDefWindowProcW;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = NULL;
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName  = L"SimpleVulkanInterop";
    wcex.hIconSm        = NULL;

    RegisterClassEx(&wcex);

    int posX = 0;
    int posY = 0;

    UINT count=0;
    int adapterIDLocal = 0;

    DISPLAY_DEVICE displayDevice;
    displayDevice.cb = sizeof(displayDevice);

    while(true)
    {
        if(EnumDisplayDevices(NULL, count, &displayDevice, 0) == FALSE)
        {
            break;
        }
        if(displayDevice.StateFlags & DISPLAY_DEVICE_ACTIVE)
        {
            if(displayDevice.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
            {
                break;
            }
            adapterIDLocal++;
        }
        count++;
    }
    EnumDisplayMonitors(NULL, NULL, MyDisplayEnumProc, (LPARAM)displayDevice.DeviceName);
    // Find adapter and provide coordinates
    g_Width =  (g_MonitorWorkArea.right - g_MonitorWorkArea.left) * 2 / 3;
    g_Height =  (g_MonitorWorkArea.bottom - g_MonitorWorkArea.top) * 2 / 3;
    posX = (g_MonitorWorkArea.left + g_MonitorWorkArea.right) / 2 - g_Width / 2;
    posY = (g_MonitorWorkArea.top + g_MonitorWorkArea.bottom) / 2 - g_Height / 2;

    // GetWindowPosition(posX, posY);
    g_hWindow = CreateWindow(L"SimpleVulkanInterop", L"SimpleVulkanInterop", WS_POPUP,
        posX, posY, g_Width, g_Height, NULL, NULL, hInstance, NULL);
    CHECK_RETURN(g_hWindow != NULL, ALVR_FAIL, L"Window failed to create");

    ::ShowWindow(g_hWindow, SW_NORMAL);
    ::UpdateWindow(g_hWindow);

    //---------------------------------------------------------------------------------------------
    // Init Vulkan (if required)
    //---------------------------------------------------------------------------------------------

    if ((RenderVulkanImage == g_RenderType) || (RenderVulkanGeometry == g_RenderType))
    {
        g_pVulkanRender = new VulkanRender(true /*render off screen*/, g_Width+100, g_Height+100);
        bool initialized = g_pVulkanRender->Init();
        CHECK_RETURN(initialized, ALVR_FAIL, L"Failed to initialize Vulkan.");

        // Create the Vulkan extension
        res = g_pFactory->CreateALVRExtensionVulkan(
            g_pVulkanRender->GetVkInstance(), NULL, &g_pExtensionVk);
        CHECK_RETURN(ALVR_OK == res, ALVR_FAIL, "CreateALVRExtensionVulkan() failed");

        // Create the Vulkan device ex
        res = g_pExtensionVk->CreateALVRDeviceExVulkan(g_pVulkanRender->GetVkDevice(), NULL, &g_pDeviceEx);
        CHECK_RETURN(ALVR_OK == res, ALVR_FAIL, "CreateALVRDeviceExVulkan() failed");
    }

    //---------------------------------------------------------------------------------------------
    // Create swap chain
    //---------------------------------------------------------------------------------------------

    res = g_D3DHelper.CreateSwapChain(g_hWindow, g_iBackbufferCount);
    CHECK_ALVR_ERROR_RETURN(res, L"CreateSwapChain() failed");

    //---------------------------------------------------------------------------------------------
    // Prepare 3D scene
    //---------------------------------------------------------------------------------------------

    res = g_D3DHelper.Create3DScene(g_Width, g_Height, true);
    CHECK_ALVR_ERROR_RETURN(res, L"Create3DScene() failed");

    if ((RenderVulkanImage == g_RenderType) || (RenderVulkanGeometry == g_RenderType))
    {
        assert(g_Width > 0);
        assert(g_Height > 0);
        // Make some test pages to copy into Vulkan and present in DX11
        g_Images.push_back(TestImage((unsigned)g_Width, (unsigned)g_Height, (unsigned)4, (BYTE)255, (BYTE)165, (BYTE)0, (BYTE)0));
        g_Images.push_back(TestImage((unsigned)g_Width, (unsigned)g_Height, (unsigned)4, (BYTE)75, (BYTE)83, (BYTE)32, (BYTE)0));
        g_Images.push_back(TestImage((unsigned)g_Width, (unsigned)g_Height, (unsigned)4, (BYTE)0, (BYTE)191, (BYTE)255, (BYTE)0));
        g_Images.push_back(TestImage((unsigned)g_Width, (unsigned)g_Height, (unsigned)4, (BYTE)184, (BYTE)68, (BYTE)6, (BYTE)0));
        g_PixelSize = 4;
    }

    std::cout << "\r" << CurrentRenderString();

    return ALVR_OK;
}

//-------------------------------------------------------------------------------------------------
ALVR_RESULT SetupVulkanRenderTarget()
{
    // Open the pBackBuffer as a Vulkan image
    if (NULL == g_VkBackBuffer)
    {
        HRESULT hr = S_OK;

        // Make a GPU texture
        D3D11_TEXTURE2D_DESC gpuTexDesc = {};
        ZeroMemory(&gpuTexDesc, sizeof(D3D11_TEXTURE2D_DESC));
        gpuTexDesc.Usage = D3D11_USAGE_DEFAULT;
        gpuTexDesc.MipLevels = 1;
        gpuTexDesc.ArraySize = 1;
        gpuTexDesc.CPUAccessFlags = 0;
        gpuTexDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        gpuTexDesc.MiscFlags |= D3D11_RESOURCE_MISC_SHARED;
        gpuTexDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        gpuTexDesc.SampleDesc.Count = 1;
        gpuTexDesc.SampleDesc.Quality = 0;
        gpuTexDesc.Width = g_Width;
        gpuTexDesc.Height = g_Height;

        D3D11_SUBRESOURCE_DATA gpuTexInitialData;
        ZeroMemory(&gpuTexInitialData, sizeof(D3D11_SUBRESOURCE_DATA));
        gpuTexInitialData.pSysMem = g_Images.current().GetImageBuffer();
        g_Images.next();
        gpuTexInitialData.SysMemPitch = g_Height*g_PixelSize;

        hr = g_D3DHelper.m_pd3dDevice->CreateTexture2D(&gpuTexDesc, &gpuTexInitialData, &g_pBackBufferSrc);
        CHECK_RETURN(SUCCEEDED(hr), ALVR_FAIL, "CreateTexture2D() failed");

        // Get a shared handle to g_pBackBufferSrc
        CComQIPtr<IDXGIResource> pDxgiRes(g_pBackBufferSrc);
        CHECK_RETURN(NULL != pDxgiRes, ALVR_FAIL, "pDxgiRes is null");

        HANDLE hShared = 0;
        pDxgiRes->GetSharedHandle(&hShared);
        CHECK_RETURN(NULL != hShared, ALVR_FAIL, "hShared is null");

        ALVRVulkanOpenSurfaceDesc vkOpenSurfaceDesc = {};
        vkOpenSurfaceDesc.sharedHandle = hShared;
        vkOpenSurfaceDesc.flags = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
        vkOpenSurfaceDesc.format = VK_FORMAT_R8G8B8A8_UNORM; // DXGI_FORMAT_R8G8B8A8_UNORM
        vkOpenSurfaceDesc.usage = 0;
        vkOpenSurfaceDesc.queueFamilyIndexCount = 0;
        vkOpenSurfaceDesc.pQueueFamilyIndices = NULL;
        ALVR_RESULT res = g_pDeviceEx->OpenSharedImage(&vkOpenSurfaceDesc, &g_VkBackBuffer);
        CHECK_RETURN(ALVR_OK == res, ALVR_FAIL, "OpenSharedImage() failed");
        CHECK_RETURN(NULL != g_VkBackBuffer, ALVR_FAIL, "vkImage for backbuffer is null");
        // We now have a Vulkan g_VkBackBuffer that we can use to render into
    }
    return ALVR_OK;
}

//-------------------------------------------------------------------------------------------------
ALVR_RESULT SetupRenderTarget()
{
    HRESULT hr = S_OK;

    CComPtr<ID3D11Texture2D> pBackBuffer;
    hr = g_D3DHelper.m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
    CHECK_HRESULT_ERROR_RETURN(hr, L"GetBuffer(0) failed");

    D3D11_TEXTURE2D_DESC backBufferDesc;
    pBackBuffer->GetDesc(&backBufferDesc);

    D3D11_RENDER_TARGET_VIEW_DESC RenderTargetViewDescription;
    ZeroMemory(&RenderTargetViewDescription, sizeof(RenderTargetViewDescription));
    RenderTargetViewDescription.Format = backBufferDesc.Format;

    RenderTargetViewDescription.ViewDimension =D3D11_RTV_DIMENSION_TEXTURE2DARRAY;             // Render target view is a Texture2D array
    RenderTargetViewDescription.Texture2DArray.MipSlice = 0;                                   // Each array element is one Texture2D
    RenderTargetViewDescription.Texture2DArray.ArraySize = 1;
    RenderTargetViewDescription.Texture2DArray.FirstArraySlice = 0;                            // First Texture2D of the array is the left eye view

    CComPtr<ID3D11RenderTargetView> pRenderTargetView;

    hr = g_D3DHelper.m_pd3dDevice->CreateRenderTargetView(pBackBuffer, &RenderTargetViewDescription, (ID3D11RenderTargetView**)&pRenderTargetView);
    CHECK_HRESULT_ERROR_RETURN(hr, L"CreateRenderTargetView() failed");

    g_D3DHelper.m_pd3dDeviceContext->OMSetRenderTargets(1, &pRenderTargetView.p, g_D3DHelper.m_pDepthStencilView);

    if ((RenderVulkanImage == g_RenderType) || (RenderVulkanGeometry == g_RenderType))
    {
        return SetupVulkanRenderTarget();
    }

    return ALVR_OK;
}

//-------------------------------------------------------------------------------------------------
ALVR_RESULT Render()
{
    if (RenderVulkanImage == g_RenderType)
    {
        // Choose a test image to write for present
        const TestImage& testImage = g_Images.current();
        g_Images.next();

        // Build staging buffer and allocate memory
        VkBuffer stagingBuffer = {};
        VkDeviceSize stagingSize = testImage.GetSize();
        VkDeviceMemory deviceMem = {};

        VkResult vkRes = VK_SUCCESS;

        VkBufferCreateInfo bufferCreateInfo = {};
        bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCreateInfo.pNext = nullptr;
        bufferCreateInfo.size = stagingSize;
        bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        vkRes = vkCreateBuffer(g_pVulkanRender->GetVkDevice(), &bufferCreateInfo, nullptr, &stagingBuffer);
        CHECK_RETURN(vkRes == VK_SUCCESS, ALVR_FAIL, "vkCreateBuffer() failed to create staging buffer");
        CHECK_RETURN(stagingBuffer != VK_NULL_HANDLE, ALVR_FAIL, "vkCreateBuffer() failed to create staging buffer");

        VkMemoryRequirements memReqs = {};
        vkGetBufferMemoryRequirements(g_pVulkanRender->GetVkDevice(), stagingBuffer, &memReqs);

        VkMemoryAllocateInfo allocationInfo = {};
        allocationInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocationInfo.pNext = nullptr;
        allocationInfo.allocationSize = memReqs.size;
        allocationInfo.memoryTypeIndex = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

        vkRes = vkAllocateMemory(g_pVulkanRender->GetVkDevice(), &allocationInfo, nullptr, &deviceMem);
        CHECK_RETURN(vkRes == VK_SUCCESS, ALVR_FAIL, "vkAllocateMemory() failed");

        vkBindBufferMemory(g_pVulkanRender->GetVkDevice(), stagingBuffer, deviceMem, 0);
        CHECK_RETURN(vkRes == VK_SUCCESS, ALVR_FAIL, "vkBindBufferMemory() failed");

        void* bufferIn = NULL;
        vkRes = vkMapMemory(g_pVulkanRender->GetVkDevice(), deviceMem, 0, testImage.GetSize(), 0, &bufferIn);
        CHECK_RETURN(vkRes == VK_SUCCESS, ALVR_FAIL, "vkMapMemory() failed");
        CHECK_RETURN(NULL != bufferIn, ALVR_FAIL, "bufferOut is NULL");
        memcpy(bufferIn, testImage.GetImageBuffer(), testImage.GetSize());
        vkUnmapMemory(g_pVulkanRender->GetVkDevice(), deviceMem);
        
        // Build command and copy buffer to image
        VkCommandPool cmdPool;
        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = g_pVulkanRender->GetVkQueueID();
        poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

        vkRes = vkCreateCommandPool(g_pVulkanRender->GetVkDevice(), &poolInfo, nullptr, &cmdPool);
        CHECK_RETURN(vkRes == VK_SUCCESS, ALVR_FAIL, L"vkCreateCommandPool() failed");

        VkCommandBuffer cmdBuffer;
        VkCommandBufferAllocateInfo cmdBufferAllocInfo = {};
        cmdBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdBufferAllocInfo.commandPool = cmdPool;
        cmdBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdBufferAllocInfo.commandBufferCount = 1;
        cmdBufferAllocInfo.pNext = nullptr;

        vkRes = vkAllocateCommandBuffers(g_pVulkanRender->GetVkDevice(), &cmdBufferAllocInfo, &cmdBuffer);
        CHECK_RETURN(vkRes == VK_SUCCESS, ALVR_FAIL, L"vkAllocatecommandBuffers() failed");

        VkCommandBufferBeginInfo cmdBufferBeginInfo = {};
        cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        cmdBufferBeginInfo.pNext = nullptr;

        vkRes = vkBeginCommandBuffer(cmdBuffer, &cmdBufferBeginInfo);
        CHECK_RETURN(vkRes == VK_SUCCESS, ALVR_FAIL, L"vkBeginCommandBuffer() failed");

        // Staging image copy
        VkImageLayout srcImageLayout = VK_IMAGE_LAYOUT_GENERAL;
        VkBufferImageCopy copyRegion = {};
        copyRegion.bufferOffset = 0;
        copyRegion.bufferRowLength = 0;
        copyRegion.bufferImageHeight = 0;
        copyRegion.imageOffset = { 0, 0, 0 };
        copyRegion.imageExtent = { g_Width, g_Height, 1 };
        copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.imageSubresource.layerCount = 1;

        vkCmdCopyBufferToImage(cmdBuffer, stagingBuffer, g_VkBackBuffer, VK_IMAGE_LAYOUT_GENERAL, 1, &copyRegion);

        vkRes = vkEndCommandBuffer(cmdBuffer);
        CHECK_RETURN(vkRes == VK_SUCCESS, ALVR_FAIL, L"vkEndCommandBuffer() failed");

        // Submit
        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.pNext = nullptr;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmdBuffer;

        vkRes = vkQueueSubmit(g_pVulkanRender->GetVkQueue(), 1, &submitInfo, VK_NULL_HANDLE);
        CHECK_RETURN(vkRes == VK_SUCCESS, ALVR_FAIL, "vkQueueSubmit() failed");
        vkRes = vkQueueWaitIdle(g_pVulkanRender->GetVkQueue());
        CHECK_RETURN(vkRes == VK_SUCCESS, ALVR_FAIL, "vkQueueWaitIdle() failed");

        // Cleanup
        vkDestroyBuffer(g_pVulkanRender->GetVkDevice(), stagingBuffer, nullptr);
        vkFreeCommandBuffers(g_pVulkanRender->GetVkDevice(), cmdPool, 1, &cmdBuffer);
        vkDestroyCommandPool(g_pVulkanRender->GetVkDevice(), cmdPool, nullptr);
        vkFreeMemory(g_pVulkanRender->GetVkDevice(), deviceMem, nullptr);

        // The Vulkan image has been rendered to. We now find the swap chain image and
        // copy the g_pBackBufferSrc texture that represents the Vulkan image to the
        // DX back buffer.  Swap chain textures cannot be shared resources so an
        // intermediate copy must be done.
        ALVR_RESULT res = ALVR_FAIL;
        HRESULT hr = S_OK;
        CComPtr<ID3D11Texture2D> pBackBuffer;
        hr = g_D3DHelper.m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
        CHECK_HRESULT_ERROR_RETURN(hr, L"GetBuffer(0) failed");

        g_D3DHelper.m_pd3dDeviceContext->CopyResource(pBackBuffer, g_pBackBufferSrc);
    }
    else if (RenderVulkanGeometry == g_RenderType)
    {
        // Draw off screen
        g_pVulkanRender->Draw();

        CHECK_RETURN(g_pVulkanRender->GetCurrentSwapChainImageIndex() >= 0, ALVR_FAIL, "Could not get current swap chain image");

        VkImage vkRenderedImage = g_pVulkanRender->GetCurrentSwapChainImage();

        VkResult vkRes = VK_SUCCESS;

        // Build command and copy the swap chain image we just created to the backbuffer
        VkCommandPool cmdPool;
        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = g_pVulkanRender->GetVkQueueID();
        poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

        vkRes = vkCreateCommandPool(g_pVulkanRender->GetVkDevice(), &poolInfo, nullptr, &cmdPool);
        CHECK_RETURN(vkRes == VK_SUCCESS, ALVR_FAIL, L"vkCreateCommandPool() failed");

        VkCommandBuffer cmdBuffer;
        VkCommandBufferAllocateInfo cmdBufferAllocInfo = {};
        cmdBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdBufferAllocInfo.commandPool = cmdPool;
        cmdBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdBufferAllocInfo.commandBufferCount = 1;
        cmdBufferAllocInfo.pNext = nullptr;

        vkRes = vkAllocateCommandBuffers(g_pVulkanRender->GetVkDevice(), &cmdBufferAllocInfo, &cmdBuffer);
        CHECK_RETURN(vkRes == VK_SUCCESS, ALVR_FAIL, L"vkAllocatecommandBuffers() failed");

        VkCommandBufferBeginInfo cmdBufferBeginInfo = {};
        cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        cmdBufferBeginInfo.pNext = nullptr;

        vkRes = vkBeginCommandBuffer(cmdBuffer, &cmdBufferBeginInfo);
        CHECK_RETURN(vkRes == VK_SUCCESS, ALVR_FAIL, L"vkBeginCommandBuffer() failed");

        // Image copy
        VkImageLayout srcImageLayout = VK_IMAGE_LAYOUT_GENERAL;
        VkImageLayout dstImageLayout = VK_IMAGE_LAYOUT_GENERAL;
        VkImageCopy copyRegion = {};
        copyRegion.srcOffset = { 0, 0, 0 };
        copyRegion.dstOffset = { 0, 0, 0 };
        copyRegion.extent = { g_Width, g_Height, 1 };;
        copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.srcSubresource.layerCount = 1;
        copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.dstSubresource.layerCount = 1;

        // Copy the Vulkan swap chain image to the back bufer image
        vkCmdCopyImage(cmdBuffer, vkRenderedImage, VK_IMAGE_LAYOUT_GENERAL, g_VkBackBuffer, VK_IMAGE_LAYOUT_GENERAL, 1, &copyRegion);

        vkRes = vkEndCommandBuffer(cmdBuffer);
        CHECK_RETURN(vkRes == VK_SUCCESS, ALVR_FAIL, L"vkEndCommandBuffer() failed");

        // Submit
        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.pNext = nullptr;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmdBuffer;

        vkRes = vkQueueSubmit(g_pVulkanRender->GetVkQueue(), 1, &submitInfo, VK_NULL_HANDLE);
        CHECK_RETURN(vkRes == VK_SUCCESS, ALVR_FAIL, "vkQueueSubmit() failed");
        vkRes = vkQueueWaitIdle(g_pVulkanRender->GetVkQueue());
        CHECK_RETURN(vkRes == VK_SUCCESS, ALVR_FAIL, "vkQueueWaitIdle() failed");

        // Cleanup
        vkFreeCommandBuffers(g_pVulkanRender->GetVkDevice(), cmdPool, 1, &cmdBuffer);
        vkDestroyCommandPool(g_pVulkanRender->GetVkDevice(), cmdPool, NULL);

        // The Vulkan image has been rendered to. We now find the swap chain image and
        // copy the g_pBackBufferSrc texture that represents the Vulkan image to the
        // back buffer.  Swap chain textures cannot be shared resources so an intermediate
        // copy must be done
        ALVR_RESULT res = ALVR_FAIL;
        HRESULT hr = S_OK;
        CComPtr<ID3D11Texture2D> pBackBuffer;
        hr = g_D3DHelper.m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
        CHECK_HRESULT_ERROR_RETURN(hr, L"GetBuffer(0) failed");

        g_D3DHelper.m_pd3dDeviceContext->CopyResource(pBackBuffer, g_pBackBufferSrc);
    }
    else
    {   // Non vulkan rendering path
        // prepare left eye
        g_D3DHelper.SetupViewportForEye(true);
        g_D3DHelper.SetupMatrixForEye(true);

        // prepare right eye
        g_D3DHelper.SetupMatrixForEye(false);

        // submit left eye 
        g_D3DHelper.SetupViewportForEye(true);
        g_D3DHelper.RenderFrame(g_iDrawRepeat);

        // submit right eye
        g_D3DHelper.SetupViewportForEye(false);
        g_D3DHelper.RenderFrame(g_iDrawRepeat);
    }

    return ALVR_OK;
}

//-------------------------------------------------------------------------------------------------
ALVR_RESULT ClearFrame(bool bLeft)
{
    CComPtr<ID3D11DepthStencilView> pDepthStencilView;
    CComPtr<ID3D11RenderTargetView> pRenderTargetView;
    g_D3DHelper.m_pd3dDeviceContext->OMGetRenderTargets(1, &pRenderTargetView, &pDepthStencilView);
    float ClearColorLeft[4] ={0.0f, 0.3f, 0.0f, 1.0f};
    float ClearColorRight[4] ={0.3f, 0.0f, 0.0f, 1.0f};
    g_D3DHelper.m_pd3dDeviceContext->ClearRenderTargetView(pRenderTargetView, bLeft ? ClearColorLeft : ClearColorRight);

    if(pDepthStencilView != NULL)
    {
        g_D3DHelper.m_pd3dDeviceContext->ClearDepthStencilView(pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0, 0);
    }
    return ALVR_OK;
}

//-------------------------------------------------------------------------------------------------
ALVR_RESULT Terminate()
{
    g_Images.clear();

    if (g_VkBackBuffer)
    {
        vkDestroyImage(g_pVulkanRender->GetVkDevice(), g_VkBackBuffer, NULL);
        g_VkBackBuffer = NULL;
    }

    if (g_pVulkanRender)
    {
        delete g_pVulkanRender;
        g_pVulkanRender = NULL;
    }

    g_pDeviceEx.Release();
    g_pExtensionVk.Release();

    g_pBackBufferSrc.Release();

    g_pFenceD3D11.Release();

    g_pLvrDevice.Release();

    ::DestroyWindow(g_hWindow);

    g_D3DHelper.Terminate();

#if defined(AFFINITY_WORK_AROUND)
    m_pLvrAffinity.Release();
#endif

    g_pFactory = NULL;

    ::FreeLibrary(g_hLiquidVRDLL);

    return ALVR_OK;
}

//-------------------------------------------------------------------------------------------------
ALVR_RESULT ChangeDrawRepeat(bool bUp)
{
    int step = 100;
    if(step == 0)
    {
        step = 1;
    }
    if(bUp)
    {
        g_iDrawRepeat += step;
    }
    else
    {
        g_iDrawRepeat -= step;
    }
    if(g_iDrawRepeat <= 0)
    {
        g_iDrawRepeat = 1;
    }
    return ALVR_OK;
}

//-------------------------------------------------------------------------------------------------
ALVR_RESULT HandleKeyboard(UINT_PTR ch)
{
    if(ch == VK_UP)
    {
        ChangeDrawRepeat(true);
    }
    else if(ch == VK_DOWN)
    {
        ChangeDrawRepeat(false);
    }
    else if (ch == 'r' || ch == 'R')
    {
        // Define a list of render types for this application
        static CircularVector<RenderType> renderTypes;
        if (renderTypes.empty())
        {
            renderTypes.push_back(RenderVulkanGeometry);
            renderTypes.push_back(RenderDX11);
            renderTypes.push_back(RenderVulkanImage);
        }
        // Terminate
        ALVR_RESULT res = Terminate();
        CHECK_ALVR_ERROR_RETURN(res, L"Terminate() failed");
        // Get next render type and set it
        g_RenderType = renderTypes.current();
        renderTypes.next();
        // Re-initialize with the new render type
        res = Init();
        CHECK_ALVR_ERROR_RETURN(res, L"Init() failed");
    }
    else
    {
        return ALVR_FALSE;
    }    
    return ALVR_OK;
}
