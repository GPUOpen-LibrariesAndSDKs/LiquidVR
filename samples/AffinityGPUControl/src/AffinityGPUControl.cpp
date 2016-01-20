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
//--------------------------------------------------------------------------------------
// File: AffinityGPUControl.cpp
//
// Starting point for LiquidVR D3D11 AffinityGPUControl Sample. 
//
// Author: Mikhail Mironov
// 
// Copyright © AMD Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "DXUT.h"
#include "DXUTmisc.h"
#include "DXUTgui.h"
#include "DXUTCamera.h"
#include "DXUTSettingsDlg.h"
#include "SDKmisc.h"
#include "SDKmesh.h"
#include "AMD_SDK.h"
#include "ShaderCacheSampleHelper.h"
#include <DirectXMath.h>
using namespace DirectX;

//--------------------------------------------------------------------------------------
// Project includes
//--------------------------------------------------------------------------------------
#include "..\inc\Resource.h"
#include "..\..\common\inc\Background.h"
#include "..\..\common\inc\Character.h"


#pragma warning( disable : 4127 ) // disable conditional expression is constant warnings
#pragma warning( disable : 4100 ) // disable unreference formal parameter warnings for /W4 builds

#define APPLICATION_TITLE L"AMD LiquidVR GPUAffinityControl v1.0.0"

//--------------------------------------------------------------------------------------
// application options
//--------------------------------------------------------------------------------------
static bool							g_bRenderHUD = true;
static bool							g_bPresentTimerStarted = false;
static bool							g_bDrawFoblinMeshes = true;

//--------------------------------------------------------------------------------------
// application objects
//--------------------------------------------------------------------------------------
static ApplicationContext			g_Context;
static Background					g_Background;
static Character					g_Character;

//--------------------------------------------------------------------------------------
// Global variables for UI
//--------------------------------------------------------------------------------------
static AMD::HUD						g_HUD;
static CDXUTDialogResourceManager	g_DialogResourceManager;    // manager for shared resources of dialogs
static CD3DSettingsDlg				g_SettingsDlg;              // Device settings dialog
static CDXUTTextHelper*				g_pTxtHelper = NULL;

//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------
enum
{
	IDC_TOGGLEFULLSCREEN = 1,
	IDC_CHANGE_DEVICE,
	IDC_STATIC_FRAMERATE,
	IDC_CHECKBOX_ENABLE_MGPU,

    IDC_STATIC_SYNC_TYPE,
    IDC_RADIO_SYNC_TYPE,
    IDC_RADIO_SYNC_TYPE_AUTOMATIC,
    IDC_RADIO_SYNC_TYPE_MANUAL,
    IDC_RADIO_SYNC_TYPE_NONE,

	IDC_CHECKBOX_DRAW_FOBLINMESH,
	IDC_CHECKBOX_ALPHA_FOBLINMESH,
	IDC_CHECKBOX_CLEAR_FOBLINMESH,
	IDC_CHECKBOX_SHIFT_FOBLINMESH,
	IDC_SLIDER_NUM_DRAW_CALLS,
	IDC_STATIC_NUM_DRAW_CALLS,
	IDC_NUM_CONTROL_IDS
};

const int AMD::g_MaxApplicationControlID = IDC_NUM_CONTROL_IDS;

//--------------------------------------------------------------------------------------
// Forward declarations 
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                          void* pUserContext );
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext );
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext );
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext );
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext );

bool CALLBACK IsD3D11DeviceAcceptable( const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo,
                                       DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext );
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
                                     void* pUserContext );
HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
                                         const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext );
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext );
void CALLBACK OnD3D11DestroyDevice( void* pUserContext );
void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime,
                                 float fElapsedTime, void* pUserContext );
HRESULT CALLBACK OnD3D11ContextCreated(ID3D11Device** ppd3dDevice, ID3D11DeviceContext** ppd3dImmediateContext);

//--------------------------------------------------------------------------------------
// helpers
//--------------------------------------------------------------------------------------
void InitApp();
void RenderText();

//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) || defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    // DXUT will create and use the best device (either D3D9 or D3D11) 
    // that is available on the system depending on which D3D callbacks are set below

    // Set DXUT callbacks
    DXUTSetCallbackMsgProc( MsgProc );
    DXUTSetCallbackKeyboard( OnKeyboard );
    DXUTSetCallbackFrameMove( OnFrameMove );
    DXUTSetCallbackDeviceChanging( ModifyDeviceSettings );

    DXUTSetCallbackD3D11DeviceAcceptable( IsD3D11DeviceAcceptable );
    DXUTSetCallbackD3D11DeviceCreated( OnD3D11CreateDevice );
    DXUTSetCallbackD3D11SwapChainResized( OnD3D11ResizedSwapChain );
    DXUTSetCallbackD3D11SwapChainReleasing( OnD3D11ReleasingSwapChain );
    DXUTSetCallbackD3D11DeviceDestroyed( OnD3D11DestroyDevice );
    DXUTSetCallbackD3D11FrameRender( OnD3D11FrameRender );
	DXUTSetCallbackD3D11ContextCreated(OnD3D11ContextCreated);

    InitApp();

    DXUTInit( true, true, NULL ); // Parse the command line, show msgboxes on error, no extra command line params
    DXUTSetCursorSettings( true, true );
	DXUTCreateWindow(APPLICATION_TITLE);

    // Require D3D_FEATURE_LEVEL_11_0
    DXUTCreateDevice( D3D_FEATURE_LEVEL_11_0, true, 1920, 1080 );

    DXUTMainLoop(); // Enter into the DXUT render loop

	// Ensure the ShaderCache aborts if in a lengthy generation process
	g_Context.m_ShaderCache.Abort();

	if (g_bPresentTimerStarted)
	{
		TIMER_End() // Present
		TIMER_Destroy()
		g_bPresentTimerStarted = false;
	}

    return DXUTGetExitCode();
}
//--------------------------------------------------------------------------------------
// Initialize the app 
//--------------------------------------------------------------------------------------
void InitApp()
{
    D3DCOLOR DlgColor = 0x88888888; // Semi-transparent background for the dialog

    g_SettingsDlg.Init( &g_DialogResourceManager );
    g_HUD.m_GUI.Init( &g_DialogResourceManager );
    g_HUD.m_GUI.SetBackgroundColors( DlgColor );
    g_HUD.m_GUI.SetCallback( OnGUIEvent );

    int iY = AMD::HUD::iElementDelta;

    g_HUD.m_GUI.AddButton( IDC_TOGGLEFULLSCREEN, L"Toggle full screen", AMD::HUD::iElementOffset, iY, AMD::HUD::iElementWidth, AMD::HUD::iElementHeight );
	g_HUD.m_GUI.AddButton(IDC_CHANGE_DEVICE, L"Change device (F2)", AMD::HUD::iElementOffset, iY += AMD::HUD::iElementDelta, AMD::HUD::iElementWidth, AMD::HUD::iElementHeight, VK_F2);

	iY += AMD::HUD::iGroupDelta / 3;

	CDXUTStatic* pFrameRateStatic = nullptr;
	g_HUD.m_GUI.AddStatic(IDC_STATIC_FRAMERATE, L"", AMD::HUD::iElementOffset, iY += AMD::HUD::iElementDelta, AMD::HUD::iElementWidth, AMD::HUD::iElementHeight, false, &pFrameRateStatic);
	D3DCOLOR frameRateColor = D3DCOLOR_RGBA(0xFF, 0, 0, 0xFF);
	pFrameRateStatic->SetTextColor(frameRateColor);

	g_HUD.m_GUI.AddCheckBox(IDC_CHECKBOX_ENABLE_MGPU, L"Enable MGPU",               AMD::HUD::iElementOffset, iY += AMD::HUD::iElementDelta, AMD::HUD::iElementWidth, AMD::HUD::iElementHeight, g_Context.m_Affinity.GetUseAffinity());

    g_HUD.m_GUI.AddStatic(IDC_STATIC_SYNC_TYPE, L"Transfer Sync", AMD::HUD::iElementOffset, iY += AMD::HUD::iElementDelta, AMD::HUD::iElementWidth, AMD::HUD::iElementHeight);

    g_HUD.m_GUI.AddRadioButton(IDC_RADIO_SYNC_TYPE_AUTOMATIC, IDC_RADIO_SYNC_TYPE, L"Automatic", AMD::HUD::iElementOffset, iY += AMD::HUD::iElementDelta, AMD::HUD::iElementWidth, AMD::HUD::iElementHeight, g_Context.m_Affinity.GetSyncType() == TRANSFER_SYNC_AUTOMATIC);
    g_HUD.m_GUI.AddRadioButton(IDC_RADIO_SYNC_TYPE_MANUAL, IDC_RADIO_SYNC_TYPE, L"Manual", AMD::HUD::iElementOffset, iY += AMD::HUD::iElementDelta, AMD::HUD::iElementWidth, AMD::HUD::iElementHeight, g_Context.m_Affinity.GetSyncType() == TRANSFER_SYNC_MANUAL);
    g_HUD.m_GUI.AddRadioButton(IDC_RADIO_SYNC_TYPE_NONE, IDC_RADIO_SYNC_TYPE, L"None", AMD::HUD::iElementOffset, iY += AMD::HUD::iElementDelta, AMD::HUD::iElementWidth, AMD::HUD::iElementHeight, g_Context.m_Affinity.GetSyncType() == TRANSFER_SYNC_NONE);


	iY += AMD::HUD::iGroupDelta / 3;

	g_HUD.m_GUI.AddCheckBox(IDC_CHECKBOX_DRAW_FOBLINMESH, L"Draw Foblin(s)",        AMD::HUD::iElementOffset, iY += AMD::HUD::iElementDelta, AMD::HUD::iElementWidth, AMD::HUD::iElementHeight, g_bDrawFoblinMeshes);
	g_HUD.m_GUI.AddCheckBox(IDC_CHECKBOX_ALPHA_FOBLINMESH, L"Alpha Foblin(s)",      AMD::HUD::iElementOffset, iY += AMD::HUD::iElementDelta, AMD::HUD::iElementWidth, AMD::HUD::iElementHeight, g_Character.GetAlpha());
	g_HUD.m_GUI.AddCheckBox(IDC_CHECKBOX_CLEAR_FOBLINMESH, L"Clear Foblin(s) Bkgs", AMD::HUD::iElementOffset, iY += AMD::HUD::iElementDelta, AMD::HUD::iElementWidth, AMD::HUD::iElementHeight, g_Character.GetClear());
	g_HUD.m_GUI.AddCheckBox(IDC_CHECKBOX_SHIFT_FOBLINMESH, L"Shift Foblin(s)",      AMD::HUD::iElementOffset, iY += AMD::HUD::iElementDelta, AMD::HUD::iElementWidth, AMD::HUD::iElementHeight, g_Character.GetShift());

	int minParam;
	int maxParam;
	g_Character.GetDrawLimits(&minParam, &maxParam);
	int currentParam = g_Character.GetTodoDraw();
	WCHAR szTemp[256];
	swprintf_s(szTemp, L"Number of Draw Calls: %i", currentParam);
	g_HUD.m_GUI.AddStatic(IDC_STATIC_NUM_DRAW_CALLS, szTemp, AMD::HUD::iElementOffset, iY += AMD::HUD::iElementDelta, AMD::HUD::iElementWidth, AMD::HUD::iElementHeight);
	g_HUD.m_GUI.AddSlider(IDC_SLIDER_NUM_DRAW_CALLS, AMD::HUD::iElementOffset, iY += AMD::HUD::iElementDelta, AMD::HUD::iElementWidth, AMD::HUD::iElementHeight, minParam, maxParam, currentParam, false);




	iY += AMD::HUD::iGroupDelta;

	g_Context.m_ShaderCache.SetShowShaderISAFlag(false);
	AMD::InitApp(g_Context.m_ShaderCache, g_HUD, iY, false);
}
//--------------------------------------------------------------------------------------
// Render the help and statistics text. This function uses the ID3DXFont interface for 
// efficient text rendering.
//--------------------------------------------------------------------------------------
void RenderText()
{
    g_pTxtHelper->Begin();
    g_pTxtHelper->SetInsertionPos( 5, 5 );
    g_pTxtHelper->SetForegroundColor( XMFLOAT4( 1.0f, 0.0f, 0.0f, 1.0f ) );
    g_pTxtHelper->DrawTextLine( DXUTGetFrameStats( DXUTIsVsyncEnabled() ) );
    g_pTxtHelper->DrawTextLine( DXUTGetDeviceStats() );

	double fTotal = TIMER_GetTime(Gpu, L"Total") * 1000.0f;
	double fFoblin = TIMER_GetTime(Gpu, L"Total|FoblinMesh") * 1000.0f;
	double fLeftEye = TIMER_GetTime(Gpu, L"Total|FoblinMesh|LeftEye") * 1000.0f;
	double fRightEye = TIMER_GetTime(Gpu, L"Total|FoblinMesh|RightEye") * 1000.0f;
	double fTransfer = TIMER_GetTime(Gpu, L"Total|Transfer") * 1000.0f;
	double fPresent = TIMER_GetTime(Gpu, L"Present") * 1000.0f;

	double fPerFoblin = fFoblin / (double)(g_Character.GetTodoDraw() * 2) * 1000.0f;

	fTotal += fPresent;
	double fOverhead = fFoblin - fLeftEye - fRightEye;
	WCHAR wcbuf[256];
	swprintf_s(wcbuf, 256, L"Cost in milliseconds( Total = %.2f, Left Eye = %.2f, Right Eye = %.2f, Transfer = %.2f, Present = %.2f, FoblinMesh = %.2f [each: %.2fus], Overhead = %.2f )", 
		fTotal, fLeftEye, fRightEye, fTransfer, fPresent, fFoblin, fPerFoblin, fOverhead);
	g_pTxtHelper->DrawTextLine( wcbuf );

    g_pTxtHelper->SetInsertionPos( 5, DXUTGetDXGIBackBufferSurfaceDesc()->Height - AMD::HUD::iElementDelta );
	g_pTxtHelper->DrawTextLine( L"Toggle GUI    : F1" );

    g_pTxtHelper->End();


}


//--------------------------------------------------------------------------------------
// Reject any D3D11 devices that aren't acceptable by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D11DeviceAcceptable( const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo,
                                       DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext )
{
	if (DeviceInfo->DeviceType != D3D_DRIVER_TYPE_HARDWARE)
	{
		return false;
	}
    return true;
}


//--------------------------------------------------------------------------------------
// Create any D3D11 resources that aren't dependant on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
                                     void* pUserContext )
{
    HRESULT hr;

    hr = DXUTGetDXGIFactory()->MakeWindowAssociation(DXUTGetHWND(), DXGI_MWA_NO_ALT_ENTER);

    ID3D11DeviceContext* pd3dImmediateContext = DXUTGetD3D11DeviceContext();
    V_RETURN( g_DialogResourceManager.OnD3D11CreateDevice( pd3dDevice, pd3dImmediateContext ) );
    V_RETURN( g_SettingsDlg.OnD3D11CreateDevice( pd3dDevice ) );
    g_pTxtHelper = new CDXUTTextHelper( pd3dDevice, pd3dImmediateContext, &g_DialogResourceManager, 15 );

    // Create AMD_SDK resources here
    g_HUD.OnCreateDevice( pd3dDevice );

	g_Context.Init(pd3dDevice, pd3dImmediateContext);
	g_Background.Init(pd3dDevice, &g_Context);
	g_Character.Init(pd3dDevice, pd3dImmediateContext, &g_Context);

	TIMER_Init(pd3dDevice)

	// Generate shaders ( this is an async operation - call AMD::ShaderCache::ShadersReady() to find out if they are complete ) 
    static bool bFirstPass = true;
    if( bFirstPass )
    {
		// Add the applications shaders to the cache
		g_Context.m_ShaderCache.GenerateShaders(AMD::ShaderCache::CREATE_TYPE_COMPILE_CHANGES);    // Only compile shaders that have changed (development mode)
        bFirstPass = false;
    } 
	g_Context.m_Affinity.SetUseAffinity(true);

	// Set toggle whether to render using Affinity MGPU
	g_HUD.m_GUI.GetCheckBox(IDC_CHECKBOX_ENABLE_MGPU)->SetChecked(g_Context.m_Affinity.GetUseAffinity());
	g_HUD.m_GUI.GetCheckBox(IDC_CHECKBOX_ENABLE_MGPU)->SetEnabled(g_Context.m_Affinity.IsEnabled());


    g_HUD.m_GUI.GetRadioButton(IDC_RADIO_SYNC_TYPE_AUTOMATIC)->SetEnabled(g_Context.m_Affinity.GetUseAffinity());
    g_HUD.m_GUI.GetRadioButton(IDC_RADIO_SYNC_TYPE_MANUAL)->SetEnabled(g_Context.m_Affinity.GetUseAffinity());
    g_HUD.m_GUI.GetRadioButton(IDC_RADIO_SYNC_TYPE_NONE)->SetEnabled(g_Context.m_Affinity.GetUseAffinity());

    wchar_t text[1000];
    swprintf_s(text, _countof(text), L"%s : %s", APPLICATION_TITLE, g_Context.m_Affinity.IsEnabled() ? L"MGPU available" : L"MGPU not available");
    ::SetWindowTextW(::DXUTGetHWND(), text);

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Create any D3D11 resources that depend on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
                                         const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
    HRESULT hr;

    V_RETURN( g_DialogResourceManager.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );
    V_RETURN( g_SettingsDlg.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );

    // Setup the camera's projection parameters
    float fAspectRatio = pBackBufferSurfaceDesc->Width / ( FLOAT )pBackBufferSurfaceDesc->Height / 2;
	g_Context.m_Camera.SetProjParams(XM_PI / 4, fAspectRatio, 0.1f, 1000.0f);
//    g_Context.m_Camera.SetProjParams(XM_PI / 4, fAspectRatio, 0.1f, 10000.0f);

	g_Context.m_Light.SetRadius(10.0f);
	g_Context.m_Light.SetLightDirection(XMFLOAT3(0.6f, 0.4f, 0.3f));
	g_Context.m_Light.SetButtonMask(MOUSE_RIGHT_BUTTON);

	g_Context.m_fbWidth = (float)pBackBufferSurfaceDesc->Width;
	g_Context.m_fbHeight = (float)pBackBufferSurfaceDesc->Height;

    // Set the location and size of the AMD standard HUD
    g_HUD.m_GUI.SetLocation( pBackBufferSurfaceDesc->Width - AMD::HUD::iDialogWidth, 0 );
    g_HUD.m_GUI.SetSize( AMD::HUD::iDialogWidth, pBackBufferSurfaceDesc->Height );
    g_HUD.OnResizedSwapChain( pBackBufferSurfaceDesc );
	
	assert( D3D_OK == hr );
	        
    return S_OK;
}

//--------------------------------------------------------------------------------------
// Render the scene using the D3D11 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime,
                                 float fElapsedTime, void* pUserContext )
{
    if (g_bPresentTimerStarted)
    {
        TIMER_End(); // Present
        // Reset the timer at start of frame
        TIMER_Reset();
        g_bPresentTimerStarted = false;
    }


    // If the settings dialog is being shown, then render it instead of rendering the app's scene
    if( g_SettingsDlg.IsActive() )
    {
        g_SettingsDlg.OnRender( fElapsedTime );
        return;
    }       
	TIMER_Begin(0, L"Total");

	g_Context.m_Affinity.SetRenderGpuMask(GPUMASK_BOTH);

    DXUTSetupD3D11Views(pd3dImmediateContext);

    // Switch off alpha blending
    float BlendFactor[1] = { 0.0f };
	pd3dImmediateContext->OMSetBlendState(g_Context.m_pOpaqueState, BlendFactor, 0xffffffff);
    pd3dImmediateContext->OMSetDepthStencilState(g_Context.m_pDepthStateOpaque, 0);
    

    // Render objects here...
	if (g_Context.m_ShaderCache.ShadersReady())
	{
		float ClearColor[4] = { 0.0f, 0.0f, 0.3f, 1.0f }; // red,green,blue,alpha

		// RenderToDirectDisplay
		ID3D11RenderTargetView* pRtv = nullptr;
		pd3dImmediateContext->OMGetRenderTargets(1, &pRtv, nullptr);
		pd3dImmediateContext->ClearRenderTargetView(pRtv, ClearColor);
		pRtv->Release();


		auto pDSV = DXUTGetD3D11DepthStencilView();
		if (pDSV != NULL)
		{
            pd3dImmediateContext->ClearDepthStencilView(pDSV, D3D11_CLEAR_DEPTH, 1.0, 0);
        }

		g_Character.Time();

//		g_Context.m_Affinity.SetRenderGpuMask(GPUMASK_BOTH);
		g_Background.Render(pd3dImmediateContext, g_Context.m_fbWidth, g_Context.m_fbHeight);

		g_Context.m_Affinity.SetRenderGpuMask(GPUMASK_BOTH);
		pDSV = DXUTGetD3D11DepthStencilView();
		if (pDSV != NULL)
		{
			pd3dImmediateContext->ClearDepthStencilView(pDSV, D3D11_CLEAR_DEPTH, 1.0, 0);
		}

		if (g_HUD.m_GUI.GetCheckBox(IDC_CHECKBOX_DRAW_FOBLINMESH)->GetChecked())
		{
			g_Character.Render(pd3dImmediateContext, g_Context.m_fbWidth, g_Context.m_fbHeight);
		}
		TIMER_Begin(0, L"Transfer");
		g_Context.m_Affinity.TransferRightEye(pd3dImmediateContext, g_Context.m_fbWidth, g_Context.m_fbHeight);
		TIMER_End() // Transfer
	}
		// Only render the HUD once on GPU0
	g_Context.m_Affinity.SetRenderGpuMask(GPUMASK_LEFT);
	D3D11_VIEWPORT viewport;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MinDepth = 0;
	viewport.MaxDepth = 1;
	viewport.Width = g_Context.m_fbWidth;
	viewport.Height = g_Context.m_fbHeight;
	pd3dImmediateContext->RSSetViewports(1, &viewport);
        	
	// Switch off alpha blending
	pd3dImmediateContext->OMSetBlendState(g_Context.m_pOpaqueState, BlendFactor, 0xffffffff);
    pd3dImmediateContext->OMSetDepthStencilState(g_Context.m_pDepthStateOpaque, 0);

	TIMER_End() // Total

    DXUT_BeginPerfEvent( DXUT_PERFEVENTCOLOR, L"HUD / Stats" );

	AMD::ProcessUIChanges();

	if (g_Context.m_ShaderCache.ShadersReady())
	{
		// update frame rate
		WCHAR szTemp[256];
		swprintf_s(szTemp, L"%.1f FPS", DXUTGetFPS());
		g_HUD.m_GUI.GetStatic(IDC_STATIC_FRAMERATE)->SetText(szTemp);

        // Render the HUD
        if( g_bRenderHUD )
        {
            g_HUD.OnRender( fElapsedTime );
        }

        RenderText();

		AMD::RenderHUDUpdates( g_pTxtHelper );
    }
	else
    {
        // Render shader cache progress if still processing
		XMFLOAT4 f4( 1.0f, 1.0f, 0.0f, 1.0f );
		g_Context.m_ShaderCache.RenderProgress(g_pTxtHelper, 15, XMLoadFloat4(&f4));
    }
    
    DXUT_EndPerfEvent();

    static DWORD dwTimefirst = GetTickCount();
    if ( GetTickCount() - dwTimefirst > 5000 )
    {    
        OutputDebugString( DXUTGetFrameStats( DXUTIsVsyncEnabled() ) );
        OutputDebugString( L"\n" );
        dwTimefirst = GetTickCount();
    }

	TIMER_Begin(0, L"Present");
	g_bPresentTimerStarted = true;
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11ResizedSwapChain 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext )
{
    g_DialogResourceManager.OnD3D11ReleasingSwapChain();

}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11CreateDevice 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11DestroyDevice( void* pUserContext )
{

	g_Background.Terminate();
	g_Character.Terminate();

    g_DialogResourceManager.OnD3D11DestroyDevice();
    g_SettingsDlg.OnD3D11DestroyDevice();
    DXUTGetGlobalResourceCache().OnDestroyDevice();
    SAFE_DELETE( g_pTxtHelper );
    // Destroy AMD_SDK resources here
	g_HUD.OnDestroyDevice();
	if (g_bPresentTimerStarted)
	{
		TIMER_End() // Present
		TIMER_Destroy()
		g_bPresentTimerStarted = false;
	}
	g_Context.Terminate();
}


//--------------------------------------------------------------------------------------
// Called right before creating a D3D9 or D3D11 device, allowing the app to modify the device settings as needed
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
	// need this to to match DirectToDisplay color space
	pDeviceSettings->d3d11.sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;


    return true;
}


//--------------------------------------------------------------------------------------
// Handle updates to the scene.  This is called regardless of which D3D API is used
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{
    // Update the camera's position based on user input 
	g_Context.m_Camera.FrameMove(fElapsedTime);
}


//--------------------------------------------------------------------------------------
// Handle messages to the application
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                          void* pUserContext )
{
    // Pass messages to dialog resource manager calls so GUI state is updated correctly
    *pbNoFurtherProcessing = g_DialogResourceManager.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    // Pass messages to settings dialog if its active
    if( g_SettingsDlg.IsActive() )
    {
        g_SettingsDlg.MsgProc( hWnd, uMsg, wParam, lParam );
        return 0;
    }

    // Give the dialogs a chance to handle the message first
    *pbNoFurtherProcessing = g_HUD.m_GUI.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;
    
    // Pass all remaining windows messages to camera so it can respond to user input
	g_Context.m_Camera.HandleMessages(hWnd, uMsg, wParam, lParam);

    // Pass all remaining windows messages to the light Object
	g_Context.m_Light.HandleMessages(hWnd, uMsg, wParam, lParam);

    return 0;
}


//--------------------------------------------------------------------------------------
// Handle key presses
//--------------------------------------------------------------------------------------
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext )
{
    if( bKeyDown )
    {
        switch( nChar )
        {
			case VK_F1:
				g_bRenderHUD = !g_bRenderHUD;
				break;
			case VK_F2:
				g_SettingsDlg.SetActive(!g_SettingsDlg.IsActive());
				break;
            case VK_RETURN:
                if (bAltDown == true)
                {
                    DXUTToggleFullScreen();
                }
                break;
		}
    }
}

//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext )
{
    switch( nControlID )
    {
        case IDC_TOGGLEFULLSCREEN:
            DXUTToggleFullScreen();
            break;
		case IDC_CHANGE_DEVICE:
            g_SettingsDlg.SetActive( !g_SettingsDlg.IsActive() );
            break;
		case IDC_SLIDER_NUM_DRAW_CALLS:
		{
			int currentParam = ((CDXUTSlider*)pControl)->GetValue();
			WCHAR szTemp[256];
			swprintf_s(szTemp, L"Number of Draw Calls: %i", currentParam);
			g_HUD.m_GUI.GetStatic(IDC_STATIC_NUM_DRAW_CALLS)->SetText(szTemp);
			g_Character.SetTodoDraw(currentParam);
			break;
		}
		case IDC_CHECKBOX_ENABLE_MGPU:
			g_Context.m_Affinity.SetUseAffinity(((CDXUTCheckBox*)pControl)->GetChecked());

            g_HUD.m_GUI.GetRadioButton(IDC_RADIO_SYNC_TYPE_AUTOMATIC)->SetEnabled(g_Context.m_Affinity.GetUseAffinity());
            g_HUD.m_GUI.GetRadioButton(IDC_RADIO_SYNC_TYPE_MANUAL)->SetEnabled(g_Context.m_Affinity.GetUseAffinity());
            g_HUD.m_GUI.GetRadioButton(IDC_RADIO_SYNC_TYPE_NONE)->SetEnabled(g_Context.m_Affinity.GetUseAffinity());

			break;

        case IDC_RADIO_SYNC_TYPE_AUTOMATIC:
            g_Context.m_Affinity.SetSyncType(TRANSFER_SYNC_AUTOMATIC);
            break;
        case IDC_RADIO_SYNC_TYPE_MANUAL:
            g_Context.m_Affinity.SetSyncType(TRANSFER_SYNC_MANUAL);
			break;
        case IDC_RADIO_SYNC_TYPE_NONE:
            g_Context.m_Affinity.SetSyncType(TRANSFER_SYNC_NONE);
            break;

		case IDC_CHECKBOX_SHIFT_FOBLINMESH:
			g_Character.SetShift(((CDXUTCheckBox*)pControl)->GetChecked());
			break;
		case IDC_CHECKBOX_ALPHA_FOBLINMESH:
			g_Character.SetAlpha(((CDXUTCheckBox*)pControl)->GetChecked());
			break;
		case IDC_CHECKBOX_CLEAR_FOBLINMESH:
			g_Character.SetClear(((CDXUTCheckBox*)pControl)->GetChecked());
			break;
		default:
			AMD::OnGUIEvent( nEvent, nControlID, pControl, pUserContext );
			break;
    }
}
//--------------------------------------------------------------------------------------
// replaces context with a wrapper
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11ContextCreated(ID3D11Device** ppd3dDevice, ID3D11DeviceContext** ppd3dImmediateContext)
{
    return g_Context.m_Affinity.Init(g_Context.m_pLiquidVRAffinity, ppd3dDevice, ppd3dImmediateContext);
}
//--------------------------------------------------------------------------------------
// EOF.
//--------------------------------------------------------------------------------------
