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
// SimpleMGPU.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <tchar.h>
#include <conio.h>
#include "../../inc/LiquidVR.h"
#include "../common/inc/D3DHelper.h"
#include "../common/inc/LvrLogger.h"

//-------------------------------------------------------------------------------------------------
enum GpuMask
{
    GPUMASK_LEFT = 0x1,
    GPUMASK_RIGHT = 0x2,
    GPUMASK_BOTH = (GPUMASK_LEFT | GPUMASK_RIGHT)
};

enum TransferSyncMode
{
    TRANSFER_SYNC_AUTOMATIC = 0,
    TRANSFER_SYNC_MANUAL    = 1,
    TRANSFER_SYNC_NONE      = 2,
};
//-------------------------------------------------------------------------------------------------
ALVR_RESULT Init();
ALVR_RESULT Terminate();
ALVR_RESULT SetupRenderTarget();
ALVR_RESULT ClearFrame();
ALVR_RESULT TransferEye();
ALVR_RESULT SetGpuAffinity(GpuMask mask);

ALVR_RESULT WaitForCompletion(bool bLeft);
ALVR_RESULT ChangeDrawRepeat(bool bUp);
bool        HandleKeyboard(UINT_PTR ch);

//-------------------------------------------------------------------------------------------------
// options
//-------------------------------------------------------------------------------------------------
static bool                                     g_bEnableMGPU = true;
static const int                                g_iBackbufferCount = 2;
//static int                                      g_iDrawRepeat = 1;
static int                                      g_iDrawRepeat = 10000; // use for GPU load
static bool                                     g_bSingleSubmission = true;
static bool                                     g_bUseCFX = true;
static TransferSyncMode                         g_eTransferSyncMode = TRANSFER_SYNC_AUTOMATIC;

//-------------------------------------------------------------------------------------------------
static D3DHelper                                g_D3DHelper;
static HWND                                     g_hWindow = NULL;

static ALVRFactory*                             g_pFactory;
static ATL::CComPtr<ALVRDeviceExD3D11>          g_pLvrDevice;
static ATL::CComPtr<ALVRGpuAffinity>            m_pLvrAffinity;
static ATL::CComPtr<ALVRMultiGpuDeviceContext>	g_pLvrDeviceContext;

static ATL::CComPtr<ALVRGpuSemaphore>           g_pTransferComplete0;
static ATL::CComPtr<ALVRGpuSemaphore>           g_pTransferComplete1;

static ATL::CComPtr<ALVRFence>                  g_pFenceD3D11;

HMODULE                                         g_hLiquidVRDLL = NULL;
static bool                                     g_bTransferFlip = false;

//-------------------------------------------------------------------------------------------------
int _tmain(int argc, _TCHAR* argv[])
{
    ALVR_RESULT res = ALVR_OK;

    printf("Type 'm' to toggle MGPU.\nType 's' to toggle single/double submission.\nType 'x' to toggle CFX.\nUse UP/DOWN keys to change frame rate.\n Type any other key to exit:\n");

    __int64 lastRenderTime = 0;
    double fAverageDuration = 0;

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
            if(HandleKeyboard(ch) == false)
            {
                break;
            }
        }
        if(PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
        {
            GetMessage(&msg, NULL, 0, 0);
            if(msg.message == WM_KEYDOWN)
            {
                if(HandleKeyboard(msg.wParam) == false)
                {
                    break;
                }
            }

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            if (lastRenderTime == 0)
            {
                lastRenderTime = high_precision_clock();
            }
            SetGpuAffinity(GPUMASK_BOTH);
            SetupRenderTarget();
            ClearFrame();

            if(g_bEnableMGPU)
            {
                if(g_bSingleSubmission && g_bUseCFX)
                {
                    // setup right eye 
                    SetGpuAffinity(GPUMASK_RIGHT);
                    g_D3DHelper.SetupViewportForEye(false);
                    g_D3DHelper.SetupMatrixForEye(false);

                    // setup left eye 
                    SetGpuAffinity(GPUMASK_LEFT);
                    g_D3DHelper.SetupViewportForEye(true);
                    g_D3DHelper.SetupMatrixForEye(true);

                    // draw cube in both GPUs
                    SetGpuAffinity(GPUMASK_BOTH);
                    g_D3DHelper.RenderFrame(g_iDrawRepeat);
                }
                else
                {
                    // submit right eye first because transfer job belongs to the right GPU engine and it will be faster
                    SetGpuAffinity(GPUMASK_RIGHT);
                    g_D3DHelper.SetupViewportForEye(false);
                    g_D3DHelper.SetupMatrixForEye(false);
                    g_D3DHelper.RenderFrame(g_iDrawRepeat);

                    // submit left eye 
                    SetGpuAffinity(GPUMASK_LEFT);
                    g_D3DHelper.SetupViewportForEye(true);
                    g_D3DHelper.SetupMatrixForEye(true);
                    g_D3DHelper.RenderFrame(g_iDrawRepeat);
                    // restore the affinity mask to both GPUs
                    SetGpuAffinity(GPUMASK_BOTH);
                }

                // transfer half of the frame from right GPU to the left GPU
                TransferEye();

                // wait for left GPU engine for completion
                SetGpuAffinity(GPUMASK_LEFT);
            }
            else
            {
                // submit right eye first because transfer job belongs to the right GPU engine and it will be faster
                g_D3DHelper.SetupViewportForEye(false);
                g_D3DHelper.SetupMatrixForEye(false);
                g_D3DHelper.RenderFrame(g_iDrawRepeat);

                // submit left eye 
                g_D3DHelper.SetupViewportForEye(true);
                g_D3DHelper.SetupMatrixForEye(true);
                g_D3DHelper.RenderFrame(g_iDrawRepeat);
            }
//          Uncomment the following line if stalling the CPU until all rendering has been completed is desired for any reason.
//          WaitForCompletion demonstrates the use of GPU fences to implement synchronization between the GPU and the CPU.
//          WaitForCompletion(true);

            __int64 endRenderTime = high_precision_clock();

            double duration = (endRenderTime - lastRenderTime) / 10000.; //ms
            lastRenderTime = endRenderTime;

            if (fAverageDuration == 0)
            {
                fAverageDuration = duration;
            }
            else
            {
                fAverageDuration = fAverageDuration + 0.05 * (duration - fAverageDuration); // simple alpha filter for values
            }
            if(g_bUseCFX)
            { 
                printf("\rCFX=ON MGPU=%s Submission=%s Draws=%d Render Time=%.2fms", g_bEnableMGPU ? "ON " : "OFF", g_bSingleSubmission ? "Single" : "Double", g_iDrawRepeat, fAverageDuration);
            }
            else
            {
                printf("\rCFX=OFF MGPU=XXX Submission=Double Draws=%d Render Time=%.2fms", g_iDrawRepeat, fAverageDuration);
            }
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

    res = g_pFactory->CreateGpuAffinity(&m_pLvrAffinity);
    CHECK_ALVR_ERROR_RETURN(res, L"CreateGpuAffinity() failed");

    if(g_bUseCFX)
    { 
        res = m_pLvrAffinity->EnableGpuAffinity(ALVR_GPU_AFFINITY_FLAGS_NONE);
//      CHECK_ALVR_ERROR_RETURN(res, L"EnableGpuAffinity() failed");
        if(res != ALVR_OK)
        { 
            g_bUseCFX = false;
            LOG_INFO(L"EnableGpuAffinity() failed");
        }
    }
    //---------------------------------------------------------------------------------------------
    // create D3D11 device
    //---------------------------------------------------------------------------------------------
    res = g_D3DHelper.CreateD3D11Device();
    CHECK_ALVR_ERROR_RETURN(res, L"CreateD3D11Device() failed");

    //---------------------------------------------------------------------------------------------
    // wrap device
    //---------------------------------------------------------------------------------------------

    if(g_bUseCFX)
    {
        CComPtr<ID3D11Device>                pd3dDeviceWrapped;
        CComPtr<ID3D11DeviceContext>         pd3dDeviceContextWrapped;

        res = m_pLvrAffinity->WrapDeviceD3D11(g_D3DHelper.m_pd3dDevice, &pd3dDeviceWrapped, &pd3dDeviceContextWrapped, &g_pLvrDeviceContext);
        CHECK_ALVR_ERROR_RETURN(res, L"WrapDeviceD3D11() failed");
        g_D3DHelper.m_pd3dDevice = pd3dDeviceWrapped;
        g_D3DHelper.m_pd3dDeviceContext = pd3dDeviceContextWrapped;
    }

    res = g_pFactory->CreateALVRDeviceExD3D11(g_D3DHelper.m_pd3dDevice, NULL, &g_pLvrDevice);
    CHECK_ALVR_ERROR_RETURN(res, L"CreateALVRDeviceExD3D11() failed");

    if(g_bUseCFX)
    {
        res = g_pLvrDeviceContext->SetGpuRenderAffinity(GPUMASK_BOTH);
        CHECK_ALVR_ERROR_RETURN(res, L"SetGpuRenderAffinity(GPUMASK_BOTH) failed");
    }
    res = g_pLvrDevice->CreateGpuSemaphore(&g_pTransferComplete0);
    CHECK_ALVR_ERROR_RETURN(res, L"CreateGpuSemaphore() failed");
    res = g_pLvrDevice->CreateGpuSemaphore(&g_pTransferComplete1);
    CHECK_ALVR_ERROR_RETURN(res, L"CreateGpuSemaphore() failed");

    res = g_pLvrDevice->CreateFence(&g_pFenceD3D11);
    CHECK_ALVR_ERROR_RETURN(res, L"CreateFence() failed");

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
    res = g_D3DHelper.Create3DScene(width, height, true);
    CHECK_ALVR_ERROR_RETURN(res, L"Create3DScene() failed");



    if(g_bUseCFX)
    {
        g_pLvrDeviceContext->MarkResourceAsInstanced(g_D3DHelper.m_pConstantBuffer);
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
ALVR_RESULT ClearFrame()
{
    CComPtr<ID3D11RenderTargetView> pRenderTargetView;
    g_D3DHelper.m_pd3dDeviceContext->OMGetRenderTargets(1, &pRenderTargetView.p, NULL);
    float ClearColor[4] ={0.0f, 0.3f, 0.0f, 1.0f};
    g_D3DHelper.m_pd3dDeviceContext->ClearRenderTargetView(pRenderTargetView, ClearColor);

    return ALVR_OK;
}
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
ALVR_RESULT TransferEye()
{
    ALVR_RESULT res = ALVR_OK;
    if(!g_bUseCFX)
    {
        return ALVR_OK;
    }

    g_D3DHelper.m_pd3dDeviceContext->Flush();


    CComPtr<ID3D11RenderTargetView> pRenderTargetView;
    g_D3DHelper.m_pd3dDeviceContext->OMGetRenderTargets(1, &pRenderTargetView.p, NULL);

    CComPtr<ID3D11Resource> pResource;
    pRenderTargetView->GetResource(&pResource);

    D3D11_RECT rect;


    if(g_D3DHelper.m_Height > g_D3DHelper.m_Width)
    {
        rect.left = 0;
        rect.top = g_D3DHelper.m_Height / 2;
        rect.right = rect.left + g_D3DHelper.m_Width;
        rect.bottom = rect.top + g_D3DHelper.m_Height / 2;
    }
    else
    {
        rect.left = g_D3DHelper.m_Width / 2;
        rect.top = 0;
        rect.right = rect.left + g_D3DHelper.m_Width / 2;
        rect.bottom = rect.top + g_D3DHelper.m_Height;

    }

    switch(g_eTransferSyncMode)
    {
    case TRANSFER_SYNC_AUTOMATIC:
        // transfer from right to left engine
        res = g_pLvrDeviceContext->TransferResource(pResource, pResource, 1, 0, 0, 0, &rect, &rect);
        break;
    case TRANSFER_SYNC_MANUAL:
        {
            // flip-flop semaphores to avoid reuse before completion of previous frame
            CComPtr<ALVRGpuSemaphore>      pTransferComplete = g_bTransferFlip ? g_pTransferComplete0 : g_pTransferComplete1;

            g_bTransferFlip = !g_bTransferFlip;
            
            // transfer from right to left engine - transfer happens in right engine
            // no need to wait for right engine 
            g_pLvrDeviceContext->TransferResourceEx(pResource, pResource, 1, 0, 0, 0, &rect, &rect, ALVR_GPU_ENGINE_3D, false);

            // left engine waits for the right engine for transfer completion before present using semaphore
            res = g_pLvrDevice->QueueSemaphoreSignal(ALVR_GPU_ENGINE_3D, 1, pTransferComplete);
            res = g_pLvrDevice->QueueSemaphoreWait(ALVR_GPU_ENGINE_3D, 0, pTransferComplete);
        }
        break;
    case TRANSFER_SYNC_NONE:
        g_pLvrDeviceContext->TransferResourceEx(pResource, pResource, 1, 0, 0, 0, &rect, &rect, ALVR_GPU_ENGINE_3D, false);
        break;

    }
    return res;
}
//-------------------------------------------------------------------------------------------------
ALVR_RESULT Terminate()
{
    g_pTransferComplete0.Release();
    g_pTransferComplete1.Release();

    g_pFenceD3D11.Release();


    g_pLvrDevice.Release();

    ::DestroyWindow(g_hWindow);

    g_D3DHelper.Terminate();

    g_pLvrDeviceContext.Release();
    m_pLvrAffinity.Release();
    g_pFactory = NULL;

    ::FreeLibrary(g_hLiquidVRDLL);

    return ALVR_OK;
}
//-------------------------------------------------------------------------------------------------
ALVR_RESULT SetGpuAffinity(GpuMask mask)
{
    if(g_pLvrDeviceContext == NULL)
    {
        return ALVR_OK;
    }
    return g_pLvrDeviceContext->SetGpuRenderAffinity(mask);
}
//-------------------------------------------------------------------------------------------------
ALVR_RESULT WaitForCompletion(bool bLeft)
{
    ALVR_RESULT res = g_pLvrDevice->SubmitFence(bLeft ? 0 : 1, g_pFenceD3D11);
    CHECK_ALVR_ERROR_RETURN(res, L"SubmitFence() failed");
    res = g_pFenceD3D11->Wait(2000);
    CHECK_ALVR_ERROR_RETURN(res, L"g_pFenceD3D11->Wait() failed");

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
bool HandleKeyboard(UINT_PTR ch)
{
    if(ch == 'M' || ch == 'm')
    {
        g_bEnableMGPU = !g_bEnableMGPU;
        SetGpuAffinity(GPUMASK_LEFT);
    }
    else if(ch == 'S' || ch == 's')
    {
        g_bSingleSubmission = !g_bSingleSubmission;
    }
    else if(ch == 'X' || ch == 'x')
    {
        // toggle CFX
        g_bUseCFX = !g_bUseCFX;
        Terminate();
        Init();

    }
    else if(ch == VK_UP)
    {
        ChangeDrawRepeat(true);
    }
    else if(ch == VK_DOWN)
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

