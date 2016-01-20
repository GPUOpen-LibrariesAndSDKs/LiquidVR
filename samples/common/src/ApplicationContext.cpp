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
#include "..\inc\ApplicationContext.h"
#include <DirectXMath.h>

using namespace DirectX;

static LPCWSTR SetupWorkDir()
{
	wchar_t wsSampleDir[MAX_PATH];
	WCHAR strExePath[MAX_PATH] = { 0 };
	WCHAR strExeName[MAX_PATH] = { 0 };
	GetModuleFileName(nullptr, strExePath, MAX_PATH);
	strExePath[MAX_PATH - 1] = 0;
	WCHAR* strLastSlash = wcsrchr(strExePath, TEXT('\\'));
	if (strLastSlash)
	{
		wcscpy_s(strExeName, MAX_PATH, &strLastSlash[1]);
		*strLastSlash = 0; // Chop the exe name from the exe path
		strLastSlash = wcsrchr(strExeName, TEXT('.')); // Chop the .exe from the exe name
		if (strLastSlash)
		{
			*strLastSlash = 0;
		}
	}
	swprintf_s(wsSampleDir, L"%s\\..\\..\\..\\%s", strExePath, strExeName);

	DXUTSetMediaSearchPath(wsSampleDir);
	AMD::SetSamplePath(wsSampleDir);
	return AMD::GetSamplePath();
}

//-------------------------------------------------------------------------------------------------
ApplicationContext::ApplicationContext() :
m_ShaderCache(AMD::ShaderCache::SHADER_AUTO_RECOMPILE_ENABLED, AMD::ShaderCache::ERROR_DISPLAY_ON_SCREEN, AMD::ShaderCache::GENERATE_ISA_ENABLED, AMD::ShaderCache::SHADER_COMPILER_EXE_INSTALLED, SetupWorkDir()),
m_pSamplePoint(NULL),
m_pSampleLinear(NULL),
m_pAlphaState(NULL),
m_pOpaqueState(NULL),
m_pDepthStateAlpha(NULL),
m_pDepthStateOpaque(NULL),
m_fbWidth(0),
m_fbHeight(0),
m_pLiquidVRFactory(NULL),
m_pLiquidVrDevice(NULL),
m_pLiquidVRAffinity(NULL),
m_hLiquidVRDLL(NULL)
{
	m_hLiquidVRDLL = LoadLibraryW(ALVR_DLL_NAME);
	if (m_hLiquidVRDLL != NULL)
	{
		ALVRInit_Fn fnInit = (ALVRInit_Fn)::GetProcAddress(m_hLiquidVRDLL, ALVR_INIT_FUNCTION_NAME);
        if (fnInit != NULL)
        {
            fnInit(ALVR_FULL_VERSION, (void**)&m_pLiquidVRFactory);

            m_pLiquidVRFactory->CreateGpuAffinity(&m_pLiquidVRAffinity);

            m_pLiquidVRAffinity->EnableGpuAffinity(ALVR_GPU_AFFINITY_FLAGS_NONE);
        }
	}
    else
    {
        WCHAR text[1000];
        wsprintf(text, L"LiquidVR SDK DLL (%s) is not found. Did you install runtime?", ALVR_DLL_NAME);
        ::MessageBoxW(DXUTGetHWND(), text, L"LiquidVR SDK", MB_OK | MB_ICONERROR);
        exit(1);
    }
}
//-------------------------------------------------------------------------------------------------
ApplicationContext::~ApplicationContext()
{
	if (m_hLiquidVRDLL != NULL)
	{
        m_pLiquidVRAffinity->DisableGpuAffinity();
        m_pLiquidVRAffinity->Release();
		::FreeLibrary(m_hLiquidVRDLL);
	}
}
//-------------------------------------------------------------------------------------------------
HRESULT ApplicationContext::Init(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext)
{
	HRESULT hr = S_OK;

    m_pLiquidVRFactory->CreateALVRDeviceExD3D11(pd3dDevice, NULL, &m_pLiquidVrDevice);
    
//	m_Affinity.Init(pd3dDevice);
	// Create sampler states for point and linear
	// Point
	D3D11_SAMPLER_DESC SamDesc;
	SamDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;

    SamDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    SamDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    SamDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;

	SamDesc.MipLODBias = 0.0f;
	SamDesc.MaxAnisotropy = 1;
	SamDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	SamDesc.BorderColor[0] = SamDesc.BorderColor[1] = SamDesc.BorderColor[2] = SamDesc.BorderColor[3] = 0;
	SamDesc.MinLOD = 0;
	SamDesc.MaxLOD = D3D11_FLOAT32_MAX;
	V_RETURN(pd3dDevice->CreateSamplerState(&SamDesc, &m_pSamplePoint));
	// Linear
	SamDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;

    SamDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    SamDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    SamDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;

	V_RETURN(pd3dDevice->CreateSamplerState(&SamDesc, &m_pSampleLinear));

	// Create blend states 
	D3D11_BLEND_DESC BlendStateDesc;
	BlendStateDesc.AlphaToCoverageEnable = FALSE;
	BlendStateDesc.IndependentBlendEnable = FALSE;
	BlendStateDesc.RenderTarget[0].BlendEnable = TRUE;
	BlendStateDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	BlendStateDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	BlendStateDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	BlendStateDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    BlendStateDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
    BlendStateDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;

	BlendStateDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	V_RETURN(pd3dDevice->CreateBlendState(&BlendStateDesc, &m_pAlphaState));
	BlendStateDesc.RenderTarget[0].BlendEnable = FALSE;
	V_RETURN(pd3dDevice->CreateBlendState(&BlendStateDesc, &m_pOpaqueState));


    // Create depth stencil state.
    D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
    ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));
    depthStencilDesc.DepthEnable = TRUE;
    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
    depthStencilDesc.StencilEnable = FALSE;
    depthStencilDesc.StencilReadMask = 0xFF;
    depthStencilDesc.StencilWriteMask = 0xFF;
    // Stencil operations if pixel is front-facing.
    depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
    depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    // Stencil operations if pixel is back-facing.
    depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
    depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

    V_RETURN(pd3dDevice->CreateDepthStencilState(&depthStencilDesc, &m_pDepthStateOpaque));

    depthStencilDesc.DepthEnable = FALSE;
    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    V_RETURN(pd3dDevice->CreateDepthStencilState(&depthStencilDesc, &m_pDepthStateAlpha));
    
	// Setup the camera's view parameters
//	    XMFLOAT3 vecEye( -8.32327, 7.48591, -2.2114 );
//	    XMFLOAT3 vecAt ( -7.61476, 6.91929, -1.79074 );
	//XMFLOAT3 vecEye(-1.0f, 8.0f, -10.0f);
	//XMFLOAT3 vecAt(1.0f, -1.0f, 1.0f);

	XMFLOAT3 vecEye(-10.0f, 15.0f, -10.0f);
	XMFLOAT3 vecAt(1.0f, -1.0f, 1.0f);
	m_Camera.SetRotateButtons(true, false, false);
	m_Camera.SetEnablePositionMovement(true);

	m_Camera.SetViewParams(XMLoadFloat3(&vecEye), XMLoadFloat3(&vecAt));
	m_Camera.SetScalers(0.005f, 100.0f);
//    m_Camera.SetScalers(0.005f, 1000.0f);

	// Create light object
	m_Light.StaticOnD3D11CreateDevice(pd3dDevice, pd3dImmediateContext);

	m_ClearView.Init(pd3dDevice, this);

    m_Affinity.CreateSyncObjects(m_pLiquidVrDevice);

	return S_OK;
}
//-------------------------------------------------------------------------------------------------
HRESULT ApplicationContext::Terminate()
{
	m_ClearView.Terminate();
	m_Light.StaticOnD3D11DestroyDevice();

	SAFE_RELEASE(m_pSamplePoint);
	SAFE_RELEASE(m_pSampleLinear);
	SAFE_RELEASE(m_pAlphaState);
	SAFE_RELEASE(m_pOpaqueState);

    SAFE_RELEASE(m_pDepthStateAlpha);
    SAFE_RELEASE(m_pDepthStateOpaque);


	m_ShaderCache.OnDestroyDevice();

	m_Affinity.Terminate();
    if (m_pLiquidVrDevice != NULL)
    {
        m_pLiquidVrDevice->Release();
        m_pLiquidVrDevice = NULL;
    }

	return S_OK;
}
//-------------------------------------------------------------------------------------------------
void ApplicationContext::OnResizedSwapChain(const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc)
{
	m_fbWidth = (float)pBackBufferSurfaceDesc->Width;
	m_fbHeight = (float)pBackBufferSurfaceDesc->Height;
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
