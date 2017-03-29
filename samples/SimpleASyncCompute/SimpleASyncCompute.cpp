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
// SimpleASyncCompute.cpp : Defines the entry point for the console application.
//

#include <SDKDDKVer.h>

#include <stdio.h>
#include <tchar.h>
#include <conio.h>
#include "../../inc/LiquidVR.h"
#include "../common/inc/D3DHelper.h"
#include "../common/inc/LvrLogger.h"
#include <DirectXColors.h>

#define AFFINITY_WORK_AROUND // remove all under this define when GXX fixes issue related to LvrResources  + MGPU

//-------------------------------------------------------------------------------------------------
// options
//-------------------------------------------------------------------------------------------------
static const int                                g_iBackbufferCount = 2;
//static int                                      g_iDrawRepeat = 1;
static int                                      g_iDrawRepeat = 1000; // use for GPU load
static int                                      g_iTextureSize = 512;
static int                                      g_iGridWidth = 100;
static int                                      g_iLineWidth = 5;
static bool                                     g_bCpuSync = false;
static bool                                     g_bHighPriorityQueue = false;
static bool                                     g_bTestSlices = false;
static bool                                     g_bTestMips = false;
static bool                                     g_bValidateBinding = false;

//-------------------------------------------------------------------------------------------------

ALVR_RESULT Init();
ALVR_RESULT Terminate();
ALVR_RESULT ClearFrame();
ALVR_RESULT RenderTexture();
ALVR_RESULT RenderFrame(bool bLeft);
ALVR_RESULT SetupRenderTarget();
ALVR_RESULT WaitForCompletion();
float GaussianDistribution(float x, float y, float rho);

//-------------------------------------------------------------------------------------------------
static D3DHelper                                g_D3DHelper;
static HWND                                     g_hWindow = NULL;

static ALVRFactory*                             g_pFactory;
static ATL::CComPtr<ALVRDeviceExD3D11>          g_pLvrDevice;
static ATL::CComPtr<ALVRComputeContext>         g_pComputeContext;
static ATL::CComPtr<ALVRFence>                  g_pFenceRender;

static ATL::CComPtr<ALVRComputeTask>            g_pGridTask;
static ATL::CComPtr<ALVRBuffer>                 g_pGridConstantBuffer;
static ATL::CComPtr<ALVRSampler>                g_pGridSampler;
static ATL::CComPtr<ALVRSurface>                g_pGridOutputSurface;
static ATL::CComPtr<ALVRSurface>                g_pGridOutputSurfaceMultiSlice;

static ATL::CComPtr<ALVRComputeTask>            g_pBlurTask;
static ATL::CComPtr<ALVRBuffer>                 g_pBlurConstantBuffer;
static ATL::CComPtr<ALVRSampler>                g_pBlurSampler;
static ATL::CComPtr<ALVRSurface>                g_pBlurOutputSurface;
static ATL::CComPtr<ALVRSurface>                g_pBlurOutputSurfaceMultiSlice;
static ATL::CComPtr<ID3D11ShaderResourceView>   g_BlurShaderResourceView;

static ATL::CComPtr<ALVRGpuSemaphore>           g_pShaderComplete[2];
static int                                      g_iShaderCompleteFlip = 0;

static int64_t                                  g_lastRenderTime = 0;
static double                                   g_fAverageDuration = 0;

static const char *text = "Type 's' to toggle CPU sync.\nType 'h' to toggle High Priorty queue.\nUse UP/DOWN keys to change frame rate.\nType any key to exit:\n";


#if defined(AFFINITY_WORK_AROUND)
static ATL::CComPtr<ALVRGpuAffinity>            m_pLvrAffinity;
#endif

HMODULE                                     g_hLiquidVRDLL = NULL;

extern const char s_pGridShaderText[];
extern const char s_pBlurShaderText[];

bool HandleKeyboard(UINT_PTR ch);

//-------------------------------------------------------------------------------------------------

int _tmain(int argc, _TCHAR* argv[])
{
    ALVR_RESULT res = ALVR_OK;

    printf(text);

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
            if (HandleKeyboard(ch) == false)
            {
                break;
            }
        }
        if(PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
        {
            GetMessage(&msg, NULL, 0, 0);
            if(msg.message == WM_KEYDOWN)
            {
                if (HandleKeyboard(msg.wParam) == false)
                {
                    break;
                }
            }

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            if(g_lastRenderTime == 0)
            {
                g_lastRenderTime = high_precision_clock();
            }
            SetupRenderTarget();
            ClearFrame();

            RenderTexture();

            RenderFrame(false);
            RenderFrame(true);

            __int64 endRenderTime = high_precision_clock();

            double duration = (endRenderTime - g_lastRenderTime) / 10000.; //ms
            g_lastRenderTime = endRenderTime;

            if (g_fAverageDuration == 0)
            {
                g_fAverageDuration = duration;
            }
            else
            {
                g_fAverageDuration = g_fAverageDuration + 0.05 * (duration - g_fAverageDuration); // simple alpha filter for values
            }

            printf("\rCPU Sync=%s HPQ=%s Draws %d Render Time=%.2fms    ", g_bCpuSync ? "ON " : "OFF", g_bHighPriorityQueue ? "ON " : "OFF", g_iDrawRepeat, g_fAverageDuration);
            fflush(stdout);

            res = g_D3DHelper.Present();

            g_D3DHelper.Animate();

        }
        Sleep(0);
    }

    Terminate();

    return 0;
}
//-------------------------------------------------------------------------------------------------
ALVR_RESULT ChangeDrawRepeat(bool bUp)
{
    int step = 100;
    if (step == 0)
    {
        step = 1;
    }
    if (bUp)
    {
        g_iDrawRepeat += step;
    }
    else
    {
        g_iDrawRepeat -= step;
    }
    if (g_iDrawRepeat <= 0)
    {
        g_iDrawRepeat = 1;
    }
    return ALVR_OK;
}
//-------------------------------------------------------------------------------------------------
bool HandleKeyboard(UINT_PTR ch)
{
    if (ch == 'S' || ch == 's')
    {
        g_bCpuSync = !g_bCpuSync;
    }
    else if (ch == 'H' || ch == 'h')
    {
        system("cls");
        printf(text);
        g_bHighPriorityQueue = !g_bHighPriorityQueue;
        Terminate();
        ALVR_RESULT res = Init();
        g_lastRenderTime = 0;
        g_fAverageDuration = 0;

    }
    else if (ch == VK_UP)
    {
        ChangeDrawRepeat(true);
    }
    else if (ch == VK_DOWN)
    {
        ChangeDrawRepeat(false);
    }
    else
    {
        return false;
    }
    return true;
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
ALVR_RESULT Init()
{
    HRESULT hr = S_OK;

    //---------------------------------------------------------------------------------------------
    // Init ALVR
    //---------------------------------------------------------------------------------------------

    ALVR_RESULT res = ALVR_OK;
    g_hLiquidVRDLL = LoadLibraryW(ALVR_DLL_NAME);
    CHECK_RETURN(g_hLiquidVRDLL != NULL, ALVR_FAIL, L"DLL " << ALVR_DLL_NAME << L" is not found");


    ALVRInit_Fn pInit = (ALVRInit_Fn)GetProcAddress(g_hLiquidVRDLL, ALVR_INIT_FUNCTION_NAME);
    res = pInit(ALVR_FULL_VERSION, (void**)&g_pFactory);
    CHECK_ALVR_ERROR_RETURN(res, ALVR_INIT_FUNCTION_NAME << L"failed");

#if defined(AFFINITY_WORK_AROUND)

    res = g_pFactory->CreateGpuAffinity(&m_pLvrAffinity);
    CHECK_ALVR_ERROR_RETURN(res, L"CreateGpuAffinity() failed");

    res = m_pLvrAffinity->EnableGpuAffinity(ALVR_GPU_AFFINITY_FLAGS_NONE);
    CHECK_ALVR_ERROR_RETURN(res, L"EnableGpuAffinity() failed");

    res = res = m_pLvrAffinity->DisableGpuAffinity();
    CHECK_ALVR_ERROR_RETURN(res, L"DisableGpuAffinity() failed");

#endif

    //---------------------------------------------------------------------------------------------
    // create D3D11 device
    //---------------------------------------------------------------------------------------------
    res = g_D3DHelper.CreateD3D11Device();
    CHECK_ALVR_ERROR_RETURN(res, L"CreateD3D11Device() failed");

    //---------------------------------------------------------------------------------------------
    // create ALVR device
    //---------------------------------------------------------------------------------------------

    res = g_pFactory->CreateALVRDeviceExD3D11(g_D3DHelper.m_pd3dDevice, NULL, &g_pLvrDevice);
    CHECK_ALVR_ERROR_RETURN(res, L"CreateALVRDeviceExD3D11() failed");

    //---------------------------------------------------------------------------------------------
    // create window
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
    wcex.lpszClassName  = L"SimpleMGPU";
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
    // find adapter and provide coordinates
    unsigned int width =  (g_MonitorWorkArea.right - g_MonitorWorkArea.left) * 2 / 3;
    unsigned int height =  (g_MonitorWorkArea.bottom - g_MonitorWorkArea.top) * 2 / 3;
    posX = (g_MonitorWorkArea.left + g_MonitorWorkArea.right) / 2 - width / 2;
    posY = (g_MonitorWorkArea.top + g_MonitorWorkArea.bottom) / 2 - height / 2;

    //    GetWindowPosition(posX, posY);
    g_hWindow = CreateWindow(L"SimpleMGPU", L"SimpleMGPU", WS_POPUP,
        posX, posY, width, height, NULL, NULL, hInstance, NULL);
    CHECK_RETURN(g_hWindow != NULL, ALVR_FAIL, L"Window failed to create");

    ::ShowWindow(g_hWindow, SW_NORMAL);
    ::UpdateWindow(g_hWindow);

    //---------------------------------------------------------------------------------------------
    // Create swap chain
    //---------------------------------------------------------------------------------------------

    res = g_D3DHelper.CreateSwapChain(g_hWindow, g_iBackbufferCount);
    CHECK_ALVR_ERROR_RETURN(res, L"CreateSwapChain() failed");

    //---------------------------------------------------------------------------------------------
    // prepare 3D scene
    //---------------------------------------------------------------------------------------------
    res = g_D3DHelper.Create3DScene(width, height, false);
    CHECK_ALVR_ERROR_RETURN(res, L"Create3DScene() failed");

    // Create ASync Compute context
    ALVRComputeContextDesc computeDesc = {};
    computeDesc.flags = g_bHighPriorityQueue ? ALVR_COMPUTE_HIGH_PRIORITY : ALVR_COMPUTE_NONE;

    res = g_pFactory->CreateComputeContext(g_pLvrDevice, 0, &computeDesc, &g_pComputeContext);
    if(res != ALVR_OK && g_bHighPriorityQueue)
    {
        LOG_INFO(L"CreateComputeContext() failed - High Priorty Queue is not available");
        
        // Reset HPQ to false
        g_bHighPriorityQueue = false;

        computeDesc.flags = ALVR_COMPUTE_NONE;
        res = g_pFactory->CreateComputeContext(g_pLvrDevice, 0, &computeDesc, &g_pComputeContext);
    }
    CHECK_ALVR_ERROR_RETURN(res, L"CreateComputeContext() failed");

    // compile shaders
    ATL::CComPtr<ID3DBlob> pVSBlob;
    ATL::CComPtr<ID3DBlob> pErrorBlob;

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;

    ALVRBufferDesc descBuff ={0};
    descBuff.apiSupport = ALVR_RESOURCE_API_ASYNC_COMPUTE;
    descBuff.bufferFlags = ALVR_BUFFER_CONSTANT;
    descBuff.cpuAccessFlags = ALVR_CPU_ACCESS_WRITE;
    descBuff.structureStride = 0;
    descBuff.format = ALVR_FORMAT_UNKNOWN;

    void *pBuffPtr = NULL;

    //---------------------------------------------------------------------------------------------
    // Create the Grid shader
    //---------------------------------------------------------------------------------------------
    hr = D3DCompile((LPCSTR)s_pGridShaderText, strlen(s_pGridShaderText), NULL, NULL, NULL, "main", "cs_5_0", dwShaderFlags, 0, &pVSBlob, &pErrorBlob);
    CHECK_HRESULT_ERROR_RETURN(hr, L"D3DCompile(CS, Grid) failed" << reinterpret_cast<char*>(pErrorBlob->GetBufferPointer()));

    res = g_pComputeContext->CreateComputeTask(ALVR_SHADER_MODEL_D3D11, 0, pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &g_pGridTask);
    CHECK_ALVR_ERROR_RETURN(res, L"CreateComputeTask(Grid) failed");

	ALVRVariantStruct bVariantStruct;
	bVariantStruct.type = ALVR_VARIANT_BOOL;
	bVariantStruct.boolValue = g_bValidateBinding;
	g_pGridTask->SetProperty(ALVR_COMPUTE_PROPERTY_VALIDATE_RESOURCE_BINDING, bVariantStruct);

    //---------------------------------------------------------------------------------------------
    // Create the Grid Buffer
    //---------------------------------------------------------------------------------------------

#pragma pack(push)
#pragma pack(1)
    struct GridParameters
    {
        unsigned int  width;
        unsigned int  height;
        unsigned int  gridWidth;
        unsigned int  lineWidth;
        //        BYTE	padding[0];
        DirectX::XMFLOAT4		colorBg;
        DirectX::XMFLOAT4		colorFg;
    };
#pragma pack(pop)

    descBuff.size = sizeof(GridParameters);

    res = g_pComputeContext->CreateBuffer(&descBuff, &g_pGridConstantBuffer);
    CHECK_ALVR_ERROR_RETURN(res, L"CreateBuffer(Grid) failed");

    res = g_pGridTask->BindConstantBuffer(0, g_pGridConstantBuffer);
    CHECK_ALVR_ERROR_RETURN(res, L"BindConstantBuffer(Grid) failed");

    GridParameters gridParameters = {};
    gridParameters.width = g_iTextureSize;
    gridParameters.height = g_iTextureSize;
    gridParameters.gridWidth = g_iGridWidth;
    gridParameters.lineWidth = g_iLineWidth;
    DirectX::XMVECTORF32 colorBg = DirectX::Colors::Red;
    XMStoreFloat4(&gridParameters.colorBg, colorBg);
    DirectX::XMVECTORF32 colorFg = DirectX::Colors::LightGray;
    XMStoreFloat4(&gridParameters.colorFg, colorFg);

    res = g_pGridConstantBuffer->Map(&pBuffPtr);
    CHECK_ALVR_ERROR_RETURN(res, L"Map(Grid) failed");
    memcpy(pBuffPtr, &gridParameters, sizeof(gridParameters));
    res = g_pGridConstantBuffer->Unmap();
    CHECK_ALVR_ERROR_RETURN(res, L"Unmap(Grid) failed");

    //---------------------------------------------------------------------------------------------
    // Create the Blur shader
    //---------------------------------------------------------------------------------------------
    pVSBlob.Release();
    pErrorBlob.Release();
    hr = D3DCompile((LPCSTR)s_pBlurShaderText, strlen(s_pBlurShaderText), NULL, NULL, NULL, "main", "cs_5_0", dwShaderFlags, 0, &pVSBlob, &pErrorBlob);
    CHECK_HRESULT_ERROR_RETURN(hr, L"D3DCompile(CS, Blur) failed" << reinterpret_cast<char*>(pErrorBlob->GetBufferPointer()));

    res = g_pComputeContext->CreateComputeTask(ALVR_SHADER_MODEL_D3D11, 0, pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &g_pBlurTask);
    CHECK_ALVR_ERROR_RETURN(res, L"CreateComputeTask(Blur) failed");

	bVariantStruct.boolValue = g_bValidateBinding;
	g_pBlurTask->SetProperty(ALVR_COMPUTE_PROPERTY_VALIDATE_RESOURCE_BINDING, bVariantStruct);

    //---------------------------------------------------------------------------------------------
    // Create the Blur Sampler
    //---------------------------------------------------------------------------------------------
    ALVRSamplerDesc samplerDesc ={};
    samplerDesc.filterMode = ALVR_FILTER_POINT;
    samplerDesc.addressU = ALVR_ADDRESS_CLAMP;
    samplerDesc.addressV = ALVR_ADDRESS_CLAMP;
    samplerDesc.addressW = ALVR_ADDRESS_CLAMP;
    samplerDesc.maxAnisotropy = 1;
    samplerDesc.borderColorType = ALVR_BORDER_COLOR_OPAQUE_BLACK;

    res = g_pComputeContext->CreateSampler(&samplerDesc, &g_pBlurSampler);
    CHECK_ALVR_ERROR_RETURN(res, L"CreateSampler(Blur) failed");

    res = g_pBlurTask->BindSampler(0, g_pBlurSampler);
    CHECK_ALVR_ERROR_RETURN(res, L"BindSampler(Blur) failed");


    //---------------------------------------------------------------------------------------------
    // Create the Blur Buffer
    //---------------------------------------------------------------------------------------------

#pragma pack(push)
#pragma pack(1)
    struct BlurParameters
    {
        unsigned int  width;
        unsigned int  height;
        BYTE	padding[8];
        DirectX::XMFLOAT4		avSampleWeights[16];
    };
#pragma pack(pop)

    descBuff.size = sizeof(BlurParameters);

    res = g_pComputeContext->CreateBuffer(&descBuff, &g_pBlurConstantBuffer);
    CHECK_ALVR_ERROR_RETURN(res, L"CreateBuffer(Blur) failed");

    res = g_pBlurTask->BindConstantBuffer(0, g_pBlurConstantBuffer);
    CHECK_ALVR_ERROR_RETURN(res, L"BindConstantBuffer(Blur) failed");

    BlurParameters blurParameters ={};
    blurParameters.width = g_iTextureSize;
    blurParameters.height = g_iTextureSize;

    float fDeviation = 3.0f;
    float fMultiplier = 1.25f;
    // Fill the center texel
    blurParameters.avSampleWeights[7].x = 1.0f * GaussianDistribution(0, 0, fDeviation);

    // Fill the right side
    for(int i = 1; i < 8; i++)
    {
        blurParameters.avSampleWeights[7 - i].x = fMultiplier * GaussianDistribution((float)i, 0, fDeviation);
        blurParameters.avSampleWeights[i + 7].x = blurParameters.avSampleWeights[7 - i].x;
    }

    res = g_pBlurConstantBuffer->Map(&pBuffPtr);
    CHECK_ALVR_ERROR_RETURN(res, L"Map(Blur) failed");
    memcpy(pBuffPtr, &blurParameters, sizeof(blurParameters));
    res = g_pBlurConstantBuffer->Unmap();
    CHECK_ALVR_ERROR_RETURN(res, L"Unmap(Blur) failed");

    //---------------------------------------------------------------------------------------------
    // create Grid shader output 
    //---------------------------------------------------------------------------------------------

    ALVRSurfaceDesc descSurf ={};
    descSurf.type = ALVR_SURFACE_2D;

    descSurf.surfaceFlags = ALVR_SURFACE_SHADER_OUTPUT | ALVR_SURFACE_SHADER_INPUT;
    descSurf.apiSupport = ALVR_RESOURCE_API_ASYNC_COMPUTE;
    descSurf.width = g_iTextureSize;
    descSurf.height = g_iTextureSize;
    descSurf.depth = 1;
    descSurf.format = ALVR_FORMAT_R8G8B8A8_UNORM;

    if(g_bTestSlices)
    {
        descSurf.sliceCount = 4;
        descSurf.mipCount = 1;
        if(g_bTestMips)
        {
            descSurf.mipCount = 4;
        }
        
        res = g_pComputeContext->CreateSurface(&descSurf, &g_pGridOutputSurfaceMultiSlice);
        CHECK_ALVR_ERROR_RETURN(res, L"CreateSurface(Grid) failed");

        ALVRChildSurfaceDesc childDesc = {};
        childDesc.startMip = 0;
        childDesc.mipCount = 1;
        if(g_bTestMips)
        {
            childDesc.startMip = 2;
        }
        childDesc.startSlice = 2;
        childDesc.sliceCount = 1;
        childDesc.shaderInputFormat = ALVR_FORMAT_UNKNOWN;  //optional, can be defined by format, put ALVR_FORMAT_UNKNOWN (0) if not needed
        childDesc.shaderOutputFormat = ALVR_FORMAT_UNKNOWN; //optional, can be defined by format, put ALVR_FORMAT_UNKNOWN (0) if not needed

        res = g_pComputeContext->CreateChildSurface(g_pGridOutputSurfaceMultiSlice, &childDesc, &g_pGridOutputSurface);
        CHECK_ALVR_ERROR_RETURN(res, L"CreateChildSurface(Grid) failed");
    }
    else
    {
        descSurf.sliceCount = 1;
        descSurf.mipCount = 1;
        res = g_pComputeContext->CreateSurface(&descSurf, &g_pGridOutputSurface);
        CHECK_ALVR_ERROR_RETURN(res, L"CreateSurface(Grid) failed");
    }



    res = g_pGridTask->BindOutput(0, g_pGridOutputSurface);
    CHECK_ALVR_ERROR_RETURN(res, L"BindOutput(Grid) failed");

    //---------------------------------------------------------------------------------------------
    // create Blur shader output 
    //---------------------------------------------------------------------------------------------

    descSurf.apiSupport = ALVR_RESOURCE_API_ASYNC_COMPUTE | ALVR_RESOURCE_API_D3D11;

    if(g_bTestSlices)
    {
        descSurf.sliceCount = 4;
        descSurf.mipCount = 1;
        if(g_bTestMips)
        {
            descSurf.mipCount = 4;
        }
        res = g_pComputeContext->CreateSurface(&descSurf, &g_pBlurOutputSurfaceMultiSlice);
        CHECK_ALVR_ERROR_RETURN(res, L"CreateSurface(Blur) failed");

        ALVRChildSurfaceDesc childDesc ={};
        childDesc.startMip = 0;
        childDesc.mipCount = 1;
        if(g_bTestMips)
        {
            childDesc.startMip = 2;
        }
        childDesc.startSlice = 2;
        childDesc.sliceCount = 1;
        childDesc.shaderInputFormat = ALVR_FORMAT_UNKNOWN;  //optional, can be defined by format, put ALVR_FORMAT_UNKNOWN (0) if not needed
        childDesc.shaderOutputFormat = ALVR_FORMAT_UNKNOWN; //optional, can be defined by format, put ALVR_FORMAT_UNKNOWN (0) if not needed

        res = g_pComputeContext->CreateChildSurface(g_pBlurOutputSurfaceMultiSlice, &childDesc, &g_pBlurOutputSurface);
        CHECK_ALVR_ERROR_RETURN(res, L"CreateChildSurface(Blur) failed");

    }
    else
    {
        descSurf.sliceCount = 1;
        descSurf.mipCount = 1;
        res = g_pComputeContext->CreateSurface(&descSurf, &g_pBlurOutputSurface);
        CHECK_ALVR_ERROR_RETURN(res, L"CreateSurface(Blur) failed");
    }
    res = g_pBlurTask->BindOutput(0, g_pBlurOutputSurface);
    CHECK_ALVR_ERROR_RETURN(res, L"BindOutput(Blur) failed");

    res = g_pBlurTask->BindInput(0, g_pGridOutputSurface);
    CHECK_ALVR_ERROR_RETURN(res, L"BindInput(Grid) failed");

    //---------------------------------------------------------------------------------------------
    // get Blur output view 
    //---------------------------------------------------------------------------------------------

    ALVRResourceD3D11 resource;
    res = g_pBlurOutputSurface->GetApiResource(ALVR_RENDER_API_D3D11, (void**)&resource);
    CHECK_ALVR_ERROR_RETURN(res, L"GetApiResource(BLur) failed");

    D3D11_SHADER_RESOURCE_VIEW_DESC shaderResDesc ={};
    shaderResDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    if(g_bTestSlices)
    {
        shaderResDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
        shaderResDesc.Texture2DArray.ArraySize = 1;
        shaderResDesc.Texture2DArray.FirstArraySlice = 2;
        shaderResDesc.Texture2DArray.MipLevels = 1;
        shaderResDesc.Texture2DArray.MostDetailedMip = 0;
        if(g_bTestMips)
        {
            shaderResDesc.Texture2DArray.MostDetailedMip = 2;
        }
    }
    else
    {
        shaderResDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        shaderResDesc.Texture2D.MipLevels = 1;
        shaderResDesc.Texture2D.MostDetailedMip = 0;
    }
    hr = g_D3DHelper.m_pd3dDevice->CreateShaderResourceView(resource.pResource, &shaderResDesc, &g_BlurShaderResourceView);
    resource.pResource->Release();
    CHECK_HRESULT_ERROR_RETURN(hr, L"CreateShaderResourceView() failed");

    g_D3DHelper.m_pd3dDeviceContext->PSSetShaderResources(0, 1, &g_BlurShaderResourceView.p);

    //---------------------------------------------------------------------------------------------
    // create sync semaphores and fences
    //---------------------------------------------------------------------------------------------
    for(int i = 0; i < _countof(g_pShaderComplete); i++)
    {
        res = g_pLvrDevice->CreateGpuSemaphore(&g_pShaderComplete[i]);
        CHECK_ALVR_ERROR_RETURN(res, L"CreateGpuSemaphore() failed");
    }

    res = g_pLvrDevice->CreateFence(&g_pFenceRender);
    CHECK_ALVR_ERROR_RETURN(res, L"CreateFence() failed");

    return ALVR_OK;
}
//-------------------------------------------------------------------------------------------------
ALVR_RESULT Terminate()
{
    g_pGridOutputSurfaceMultiSlice.Release();
    g_pGridOutputSurface.Release();
    g_pGridConstantBuffer.Release();
    g_pGridSampler.Release();
    g_pGridTask.Release();

    g_pBlurOutputSurfaceMultiSlice.Release();
    g_pBlurOutputSurface.Release();
    g_pBlurConstantBuffer.Release();
    g_pBlurSampler.Release();
    g_pBlurTask.Release();

    g_BlurShaderResourceView.Release();

    g_pFenceRender.Release();

    for(int i = 0; i < _countof(g_pShaderComplete); i++)
    {
        g_pShaderComplete[i].Release();
    }

    g_pComputeContext.Release();

    ::DestroyWindow(g_hWindow);

    g_D3DHelper.Terminate();

    g_pLvrDevice.Release();

#if defined(AFFINITY_WORK_AROUND)
    m_pLvrAffinity.Release();
#endif
    g_pFactory = NULL;

    ::FreeLibrary(g_hLiquidVRDLL);

    return ALVR_OK;
}
//-------------------------------------------------------------------------------------------------
ALVR_RESULT ClearFrame()
{
    CComPtr<ID3D11RenderTargetView> pRenderTargetView;
    g_D3DHelper.m_pd3dDeviceContext->OMGetRenderTargets(1, &pRenderTargetView.p, NULL);
    float ClearColor[4] ={0.0f, 0.3f, 0.0f, 1.0f};
    g_D3DHelper.m_pd3dDeviceContext->ClearRenderTargetView(pRenderTargetView, ClearColor);

    return ALVR_OK;
}
//-------------------------------------------------------------------------------------------------
ALVR_RESULT RenderTexture()
{
    if(g_bCpuSync)
    {
        // Submit fence to compute queue for CPU to wait for 3D queue on GPU to finish before executing shader tasks
        WaitForCompletion();
    }
    static const int THREAD_GROUP_SIZE = 8; // the same as in shaders

    ALVR_RESULT res = ALVR_OK;

    ALVRPoint3D offset ={0, 0, 0};
    ALVRSize3D size ={(g_iTextureSize + THREAD_GROUP_SIZE - 1) / THREAD_GROUP_SIZE, (g_iTextureSize + THREAD_GROUP_SIZE - 1) / THREAD_GROUP_SIZE, 0};

    res = g_pComputeContext->QueueTask(g_pGridTask, &offset, &size);
    CHECK_ALVR_ERROR_RETURN(res, L"QueueTask(Grid) failed");

    res = g_pComputeContext->QueueTask(g_pBlurTask, &offset, &size);
    CHECK_ALVR_ERROR_RETURN(res, L"QueueTask(Blur) failed");

    if(g_bCpuSync)
    {
        g_pComputeContext->Flush(g_pFenceRender);
        g_pFenceRender->Wait(2000); // CPU waits until compute queue is complete
    }
    else
    {
        ATL::CComPtr<ALVRGpuSemaphore>           pShaderComplete = g_pShaderComplete[g_iShaderCompleteFlip];
        g_iShaderCompleteFlip++;
        if(g_iShaderCompleteFlip == _countof(g_pShaderComplete))
        {
            g_iShaderCompleteFlip = 0;
        }
        g_pComputeContext->Flush(NULL);
        // Use semaphore to sync between compute queue and 3D queue on GPU
        g_pComputeContext->QueueSemaphoreSignal(pShaderComplete);
        g_pLvrDevice->QueueSemaphoreWait(ALVR_GPU_ENGINE_3D, 0, pShaderComplete);
    }
    return res;
}
//-------------------------------------------------------------------------------------------------
ALVR_RESULT RenderFrame(bool bLeft)
{
    g_D3DHelper.SetupViewportForEye(bLeft);
    g_D3DHelper.SetupMatrixForEye(bLeft);
    return g_D3DHelper.RenderFrame(g_iDrawRepeat);
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

    RenderTargetViewDescription.ViewDimension =D3D11_RTV_DIMENSION_TEXTURE2DARRAY;            // render target view is a Texture2D array
    RenderTargetViewDescription.Texture2DArray.MipSlice = 0;                                   // each array element is one Texture2D
    RenderTargetViewDescription.Texture2DArray.ArraySize = 1;
    RenderTargetViewDescription.Texture2DArray.FirstArraySlice = 0;                            // first Texture2D of the array is the left eye view

    CComPtr<ID3D11RenderTargetView> pRenderTargetView;

    hr = g_D3DHelper.m_pd3dDevice->CreateRenderTargetView(pBackBuffer, &RenderTargetViewDescription, (ID3D11RenderTargetView**)&pRenderTargetView);
    CHECK_HRESULT_ERROR_RETURN(hr, L"CreateRenderTargetView() failed");


    g_D3DHelper.m_pd3dDeviceContext->OMSetRenderTargets(1, &pRenderTargetView.p, NULL);

    return ALVR_OK;
}
//-------------------------------------------------------------------------------------------------
const char s_pGridShaderText[] = 
"RWTexture2D<float4> outImage : register(u0);                                                           \n"
"                                                                                                       \n"
"cbuffer Parameters : register(b0)                                                                      \n"
"{                                                                                                      \n"
"    uint  width;                                                                                       \n"
"    uint  height;                                                                                      \n"
"    uint  gridWidth;                                                                                   \n"
"    uint  lineWidth;                                                                                   \n"
"    float4 colorBg;                                                                                    \n"
"    float4 colorFg;                                                                                    \n"
"};                                                                                                     \n"
"                                                                                                       \n"
"[numthreads(8, 8, 1)]                                                                                  \n"
"void main(uint3 coord : SV_DispatchThreadID)                                                           \n"
"{                                                                                                      \n"
"    if((coord.x < width) && (coord.y < height))                                                        \n"
"    {                                                                                                  \n"
"        float4 colorOut = colorBg;                                                                     \n"
"        uint  x = coord.x % gridWidth;                                                                 \n"
"        uint  y = coord.y % gridWidth;                                                                 \n"
"        if(y < lineWidth || x < lineWidth)                                                             \n"
"        {                                                                                              \n"
"            colorOut = colorFg;                                                                        \n"
"        }                                                                                              \n"
"        outImage[coord.xy] = colorOut;                                                                 \n"
"    }                                                                                                  \n"
"}                                                                                                      \n"
;
//-------------------------------------------------------------------------------------------------
const char s_pBlurShaderText[] =
"SamplerState LinearSampler                                                                             \n"
"{                                                                                                      \n"
"   Filter = MIN_MAG_MIP_LINEAR;                                                                        \n"
"   AddressU = Clamp;                                                                                   \n"
"   AddressV = Clamp;                                                                                   \n"
"};                                                                                                     \n"
"                                                                                                       \n"
"Texture2D<float4> inImage : register(t0);                                                              \n"
"RWTexture2D<float4> outImage : register(u0);                                                           \n"
"                                                                                                       \n"
"cbuffer DataLayout : register(b0)                                                                      \n"
"{                                                                                                      \n"
"    uint  width;                                                                                       \n"
"    uint  height;                                                                                      \n"
"    float4 avSampleWeights[16];                                                                        \n"
"};                                                                                                     \n"
"// fake constant buffer to test validation - see  g_bValidateBinding                                   \n"
"cbuffer Test : register(b1)                                                                            \n"
"{                                                                                                      \n"
"    uint  tmp;                                                                                         \n"
"};                                                                                                     \n"
"                                                                                                       \n"
"[numthreads(8, 8, 1)]                                                                                  \n"
"void main(uint3 coord : SV_DispatchThreadID)                                                           \n"
"{                                                                                                      \n"
"    if((coord.x < width) && (coord.y < height))                                                        \n"
"    {                                                                                                  \n"
"       uint2 inImageSize;                                                                              \n"
"       inImage.GetDimensions(inImageSize.x, inImageSize.y);                                            \n"
"                                                                                                       \n"
"        float4 vOut[15];                                                                               \n"
"        for(int i = 0; i < 15; i++)                                                                    \n"
"        {                                                                                              \n"
"            vOut[i] = 0.0;                                                                             \n"
"        }                                                                                              \n"
"        int kernelsize = 15;                                                                           \n"
"        for(int y = 0; y <kernelsize; y++)                                                             \n"
"        {                                                                                              \n"
"            for(int x = 0; x <kernelsize; x++)                                                         \n"
"            {                                                                                          \n"
"                uint2 pos = uint2(x - kernelsize / 2 + coord.x, y - kernelsize / 2 + coord.y);         \n"
"                float2 coordf = float2((float)pos.x / inImageSize.x, (float)pos.y / inImageSize.y);    \n"
"                float4 color = inImage.SampleLevel(LinearSampler, coordf, 0);                          \n"
"                vOut[y] += color * avSampleWeights[x].x;                                               \n"
"            }                                                                                          \n"
"        }                                                                                              \n"
"        float4 vGaussinOut = 0.0;                                                                      \n"
"        for(int j = 0; j < kernelsize; j++)                                                            \n"
"        {                                                                                              \n"
"            vGaussinOut += vOut[j] * avSampleWeights[j].x;                                             \n"
"        }                                                                                              \n"
"        outImage[coord.xy] = vGaussinOut;                                                              \n"
"    }                                                                                                  \n"
"}                                                                                                      \n"
;
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static float GaussianDistribution(float x, float y, float rho)
{
    float g = 1.0f / sqrtf(2.0f * DirectX::XM_PI * rho * rho);
    g *= expf(-(x * x + y * y) / (2 * rho * rho));

    return g;
}
//-------------------------------------------------------------------------------------------------
ALVR_RESULT WaitForCompletion()
{
    // Submit a fence to the compute queue to indicate the CPU must wait before continuing
    ALVR_RESULT res = g_pLvrDevice->SubmitFence(0, g_pFenceRender);
    CHECK_ALVR_ERROR_RETURN(res, L"SubmitFence() failed");
    res = g_pFenceRender->Wait(2000);
    CHECK_ALVR_ERROR_RETURN(res, L"g_pFenceD3D11->Wait() failed");

    return ALVR_OK;
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------


