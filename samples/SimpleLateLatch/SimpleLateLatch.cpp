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
// SimpleLateLatch.cpp : Defines the entry point for the console application.
//

#include <SDKDDKVer.h>
#include <stdio.h>
#include <tchar.h>
#include <conio.h>
#include "../../inc/LiquidVR.h"
#include "../common/inc/D3DHelper.h"
#include "../common/inc/LvrLogger.h"
#include "MouseInput.h"

using namespace DirectX;

// enable this define for drivers before 15.30 Crimson 
//#define AFFINITY_WORK_AROUND // remove all under this define when GXX fixes issue related to LvrResources  + MGPU

//-------------------------------------------------------------------------------------------------
ALVR_RESULT Init();
ALVR_RESULT Terminate();
ALVR_RESULT SetupRenderTarget();
ALVR_RESULT ClearFrame(bool bLeft);
ALVR_RESULT HandleKeyboard(UINT_PTR ch);

ALVR_RESULT ChangeDrawRepeat(bool bUp);

ALVR_RESULT WaitForCompletion(bool bLeft);
//-------------------------------------------------------------------------------------------------
class Locker
{
    CRITICAL_SECTION    *m_pSect;
public:
    Locker(CRITICAL_SECTION                            *pSect) : m_pSect(pSect)
    {
        EnterCriticalSection(m_pSect);
    }
    ~Locker()
    {
        LeaveCriticalSection(m_pSect);
    }
};



class D3DHelperLateLatch : public D3DHelper,  public MouseInputCallback
{
public:
    D3DHelperLateLatch() : m_PosX(0), m_PosY(0)
    {
        InitializeCriticalSection(&m_Sect);
    }
    ~D3DHelperLateLatch()
    {
        DeleteCriticalSection(&m_Sect);
    }
    virtual ALVR_RESULT Terminate();
    virtual ALVR_RESULT SetupMatrixForEye(bool bLeft);
    virtual ALVR_RESULT QueueLatch(bool bLeft);
    virtual ALVR_RESULT BindBuffer(bool bLeft);
    // MouseInputCallback
    virtual void MouseEvent(LONG posX, LONG posY, bool bLeftDown, bool bRightDown);
    virtual ALVR_RESULT CreateConstantBuffer();
protected:
    virtual const char *GetShaderText(bool bColorCube);
    virtual ALVR_RESULT UpdateConstantBuffer(bool bLeft, DirectX::XMMATRIX &worldViewProjection);

    CComPtr<ALVRLateLatchConstantBufferD3D11>   m_pLateLatchBufferLeft;
    CComPtr<ALVRLateLatchConstantBufferD3D11>   m_pLateLatchBufferRight;
    float                                       m_PosX;
    float                                       m_PosY;

    CRITICAL_SECTION                            m_Sect;
};
//-------------------------------------------------------------------------------------------------
// options
//-------------------------------------------------------------------------------------------------
static const int                                g_iBackbufferCount = 2;
//static int                                      g_iDrawRepeat = 1;
//static int                                      g_iDrawRepeat = 100000; // use for GPU load
static volatile int                             g_iDrawRepeat = 50000; // use for GPU load
static volatile bool                            g_bLateLatchRight = false;
static volatile bool                            g_bRotate = false;

//-------------------------------------------------------------------------------------------------
static D3DHelperLateLatch                       g_D3DHelper;
static MouseInput                               g_MouseInput;
static HWND                                     g_hWindow = NULL;

static ALVRFactory*                             g_pFactory;
static ATL::CComPtr<ALVRDeviceExD3D11>          g_pLvrDevice;

#if defined(AFFINITY_WORK_AROUND)
static ATL::CComPtr<ALVRGpuAffinity>            m_pLvrAffinity;
#endif


static ATL::CComPtr<ALVRFence>                  g_pFenceD3D11;

HMODULE                                         g_hLiquidVRDLL = NULL;

//-------------------------------------------------------------------------------------------------
int _tmain(int argc, _TCHAR* argv[])
{
    ALVR_RESULT res = ALVR_OK;

    printf("Press 'L' to toggle LateLatch.\nPress 'A' to toggle rotation.\nPress 'E' to toggle mouse emulation and use drag cube up/down with mouse.\nUse UP/DOWN keys to change frame rate.\nType any other key to exit:\n");

    increase_timer_precision();
    res = Init();
    if(res != ALVR_OK)
    {
        Terminate();
        return -1;
    }
    __int64 renderTime = high_precision_clock();
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
            SetupRenderTarget();
            ClearFrame(true);

            // prepare left eye
            g_D3DHelper.SetupViewportForEye(true);
            g_D3DHelper.SetupMatrixForEye(true);
            g_D3DHelper.QueueLatch(true);

            // prepare right eye
            g_D3DHelper.SetupMatrixForEye(false);
            g_D3DHelper.QueueLatch(false);

            // submit left eye 
            g_D3DHelper.SetupViewportForEye(true);
            g_D3DHelper.BindBuffer(true);
            g_D3DHelper.RenderFrame(g_iDrawRepeat);

            // submit right eye
            g_D3DHelper.SetupViewportForEye(false);
            g_D3DHelper.BindBuffer(false);
            g_D3DHelper.RenderFrame(g_iDrawRepeat);

            // present a full frame for both eyes
            res = g_D3DHelper.Present();

            if (g_bRotate == true)
            {
                g_D3DHelper.Animate();
            }

            int64_t currentTime = high_precision_clock();
            
            printf("\rLate Latch=%s Draws %d Render Time=%.2fms", g_bLateLatchRight ? "ON " : "OFF", g_iDrawRepeat, (currentTime - renderTime) / 10000.);
            fflush(stdout);
            renderTime = currentTime;
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

    g_MouseInput.Init((HINSTANCE)GetModuleHandle(NULL), g_hWindow, &g_D3DHelper, 200);
    g_MouseInput.SetEmulation(true);

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


    g_D3DHelper.m_pd3dDeviceContext->OMSetRenderTargets(1, &pRenderTargetView.p, g_D3DHelper.m_pDepthStencilView);

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
    g_MouseInput.Terminate();

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
ALVR_RESULT WaitForCompletion(bool bLeft)
{
    ALVR_RESULT res = g_pLvrDevice->SubmitFence(bLeft ? 0 : 1, g_pFenceD3D11);
    CHECK_ALVR_ERROR_RETURN(res, L"SubmitFence() failed");
    res = g_pFenceD3D11->Wait(2000);
    CHECK_ALVR_ERROR_RETURN(res, L"g_pFenceD3D11->Wait() failed");

    return ALVR_OK;
}
//-------------------------------------------------------------------------------------------------
// D3DHelperLateLatch
//-------------------------------------------------------------------------------------------------
ALVR_RESULT D3DHelperLateLatch::Terminate()
{

    m_pLateLatchBufferLeft.Release();
    m_pLateLatchBufferRight.Release();
    return D3DHelper::Terminate();
}
//-------------------------------------------------------------------------------------------------
const char *D3DHelperLateLatch::GetShaderText(bool bColorCube)
{
    static const char s_pShaderText[] =
        "//--------------------------------------------------------------------------------------\n"
        "// Constant Buffer Variables                                                            \n"
        "//--------------------------------------------------------------------------------------\n"
        "cbuffer WorldViewProjectionArray : register( b0 )                                       \n"
        "{                                                                                       \n"
        "    matrix m_WorldViewProjection[1024];                                                 \n"
        "};                                                                                      \n"
        "//--------------------------------------------------------------------------------------\n"
        "cbuffer HeadPointer : register( b1 )                                                    \n"
        "{                                                                                       \n"
        "  unsigned int m_Head;                                                                  \n"
        "};                                                                                      \n"
        "//--------------------------------------------------------------------------------------\n"
        "struct VS_OUTPUT                                                                        \n"
        "{                                                                                       \n"
        "    float4 Pos : SV_POSITION;                                                           \n"
        "    float4 Color : COLOR0;                                                              \n"
        "};                                                                                      \n"
        "//--------------------------------------------------------------------------------------\n"
        "// Vertex Shader                                                                        \n"
        "//--------------------------------------------------------------------------------------\n"
        "VS_OUTPUT VS( float4 Pos : POSITION, float4 Color : COLOR )                             \n"
        "{                                                                                       \n"
        "    matrix data = m_WorldViewProjection[m_Head];                                        \n"
        "    VS_OUTPUT output = (VS_OUTPUT)0;                                                    \n"
        "    output.Pos = mul(Pos, data);                                                        \n"
        "    output.Color = Color;                                                               \n"
        "    return output;                                                                      \n"
        "}                                                                                       \n"
        "//--------------------------------------------------------------------------------------\n"
        "// Pixel Shader                                                                         \n"
        "//--------------------------------------------------------------------------------------\n"
        "float4 PS( VS_OUTPUT input ) : SV_Target                                                \n"
        "{                                                                                       \n"
        "    return input.Color;                                                                 \n"
        "}                                                                                       \n"
        "//--------------------------------------------------------------------------------------\n"
        ;
    return s_pShaderText;
}
//-------------------------------------------------------------------------------------------------
ALVR_RESULT D3DHelperLateLatch::CreateConstantBuffer()
{
    ALVR_RESULT res = ALVR_OK;

    Locker lock(&m_Sect);

    m_pLateLatchBufferLeft.Release();
    m_pLateLatchBufferRight.Release();

    res = g_pLvrDevice->CreateLateLatchConstantBufferD3D11(sizeof(ConstantBuffer), 1024, 0, &m_pLateLatchBufferLeft);
    CHECK_ALVR_ERROR_RETURN(res, L"CreateLateLatchConstantBufferD3D11() failed");


    res = g_pLvrDevice->CreateLateLatchConstantBufferD3D11(sizeof(ConstantBuffer), 1024, 0, &m_pLateLatchBufferRight);
    CHECK_ALVR_ERROR_RETURN(res, L"CreateLateLatchConstantBufferD3D11() failed");


    return ALVR_OK;
}
//-------------------------------------------------------------------------------------------------
ALVR_RESULT D3DHelperLateLatch::UpdateConstantBuffer(bool bLeft, DirectX::XMMATRIX &worldViewProjection)
{
    Locker lock(&m_Sect);

    CComPtr<ALVRLateLatchConstantBufferD3D11>   pLateLatchBuffer = bLeft ? m_pLateLatchBufferLeft : m_pLateLatchBufferRight;
    ALVR_RESULT res = pLateLatchBuffer->Update(&worldViewProjection, 0, sizeof(ConstantBuffer));
    CHECK_ALVR_ERROR_RETURN(res, L"Update() failed");
    return ALVR_OK;
}
//-------------------------------------------------------------------------------------------------
ALVR_RESULT D3DHelperLateLatch::BindBuffer(bool bLeft)
{
    Locker lock(&m_Sect);
    CComPtr<ALVRLateLatchConstantBufferD3D11>   pLateLatchBuffer = bLeft ? m_pLateLatchBufferLeft : m_pLateLatchBufferRight;
    ID3D11Buffer *pData[2];
    pData[0] = pLateLatchBuffer->GetDataD3D11();
    pData[1] = pLateLatchBuffer->GetIndexD3D11();
    m_pd3dDeviceContext->VSSetConstantBuffers(0, 2, pData);
    return ALVR_OK;
}
//-------------------------------------------------------------------------------------------------
ALVR_RESULT D3DHelperLateLatch::QueueLatch(bool bLeft)
{
    Locker lock(&m_Sect);
    CComPtr<ALVRLateLatchConstantBufferD3D11>   pLateLatchBuffer = bLeft ? m_pLateLatchBufferLeft : m_pLateLatchBufferRight;

    pLateLatchBuffer->QueueLatch();
    return ALVR_OK;
}
//-------------------------------------------------------------------------------------------------
void D3DHelperLateLatch::MouseEvent(LONG posX, LONG posY, bool bLeftDown, bool bRightDown)
{
    if(!bLeftDown)
    {
        return;
    }
    RECT client;
    ::GetClientRect(g_hWindow, &client);
    POINT pt = {posX, posY};
    ::ScreenToClient(g_hWindow, &pt);
    if(pt.x < client.left || pt.x >= client.right || pt.y < client.top || pt.y >= client.bottom)
    {
        return;
    }
    bool bPortrait = m_Height > m_Width;

    float PosX = 0;
    float PosY = 0;

    if(bPortrait)
    {
        if(pt.y < m_Height / 2.0f)
        { // left eye

        }
        else
        { // right eye
            pt.y -= (LONG)(m_Height / 2.0f);
        }
       PosX = 2.0f * ((float)pt.x / (m_Width) - 0.5f);
       PosY = 2.0f * ((float)pt.y / (m_Height / 2.0f) - 0.5f);
    }
    else
    {
        if(pt.x < m_Width / 2.0f)
        { // left eye

        }
        else
        { // right eye
            pt.x -= (LONG)(m_Width / 2.0f);
        }

       PosX = 2.0f * ((float)pt.x / (m_Width / 2.0f) - 0.5f);
       PosY = 2.0f * ((float)pt.y / m_Height - 0.5f);
    }


    if(m_PosX == PosX && m_PosY == PosY)
    {
        return;
    }
    m_PosX = PosX;
    m_PosY = PosY;

    if(g_bLateLatchRight)
    {
        g_D3DHelper.SetupMatrixForEye(false);
    }
}
//-------------------------------------------------------------------------------------------------
ALVR_RESULT D3DHelperLateLatch::SetupMatrixForEye(bool bLeft)
{
    ALVR_RESULT res = ALVR_OK;
    HRESULT hr = S_OK;
    XMMATRIX world = XMMatrixRotationRollPitchYaw(m_fAnimation, -m_fAnimation, 0);
    //---------------------------------------------------------------------------------------------
    // Setup matrixes
    //---------------------------------------------------------------------------------------------
    bool bPortrait = m_Width < m_Height;
    float width = bPortrait ? (float)m_Width : (float)m_Width / 2.f;
    float height = bPortrait ? (float)m_Height / 2.f : (float)m_Height;

//    float eyeOffset = 0.5f;
    float eyeOffset = 0.0f;
    XMVECTOR Eye = bLeft ? XMVectorSet(-eyeOffset, 1.0f, -5.0f, 0.0f) : XMVectorSet(eyeOffset, 1.0f, -5.0f, 0.0f);
    XMVECTOR At = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMMATRIX view = XMMatrixLookAtLH(Eye, At, Up);
    XMMATRIX projection = XMMatrixPerspectiveFovLH(XM_PIDIV2, width / height, 0.01f, 100.0f);

    if(m_PosX != 0 || m_PosY != 0)
    {
        projection = projection * XMMatrixTranslation(m_PosX, -m_PosY, 0);
    }

    if(bLeft)
    {
        projection = projection * XMMatrixTranslation(0.8f, 0, 0);
    }
    else
    {
        projection = projection * XMMatrixTranslation(-0.8f, 0, 0);
    }


    XMMATRIX worldViewProjection = world * view * projection;
    if(bPortrait)
    {
        worldViewProjection = worldViewProjection *XMMatrixRotationZ(XM_PIDIV2);
    }
    XMMATRIX worldViewProjectionTransponded = XMMatrixTranspose(worldViewProjection);;
    return UpdateConstantBuffer(bLeft, worldViewProjectionTransponded);
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
    if(ch == 'E' || ch == 'e')
    {
        g_MouseInput.SetEmulation(!g_MouseInput.GetEmulation());
    }
    else if(ch == 'L' || ch == 'l')
    {
        g_bLateLatchRight = !g_bLateLatchRight;
    }
    else if(ch == 'A' || ch == 'a')
    {
        g_bRotate = !g_bRotate;
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
        return ALVR_FALSE;
    }    
    return ALVR_OK;
}
//-------------------------------------------------------------------------------------------------

