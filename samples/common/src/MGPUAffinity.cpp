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
#include "..\inc\MGPUAffinity.h"

#ifdef _M_AMD64
#define DXX_DLL_NAME "atidxx64.dll"
#else
#define DXX_DLL_NAME "atidxx32.dll"
#endif

//-------------------------------------------------------------------------------------------------
MGPUAffinity::MGPUAffinity() :
    m_pLiquidVrDevice(NULL),
	m_pLiquidVrDeviceContext(NULL),
	m_bUseAffinity(true),
	m_eCurrentMask(GPUMASK_BOTH),
    m_eTransferSyncMode(TRANSFER_SYNC_AUTOMATIC),
    m_pRenderComplete0(NULL),
    m_pTransferComplete0(NULL),
    m_pRenderComplete1(NULL),
    m_pTransferComplete1(NULL),
    m_bFrameLoop(false)

{
}
//-------------------------------------------------------------------------------------------------
MGPUAffinity::~MGPUAffinity()
{
	Terminate();
}
//-------------------------------------------------------------------------------------------------
HRESULT MGPUAffinity::Init(ALVRGpuAffinity *pAffinity, ID3D11Device** ppd3dDevice, ID3D11DeviceContext** ppd3dImmediateContext)
{
    if ((ID3D11DeviceContext*)m_pLiquidVrDeviceContext == *ppd3dImmediateContext)
	{
		return S_OK;
	}
	Terminate();
    bool bUseAffinity = m_bUseAffinity;
	m_bUseAffinity = false;

	ID3D11DeviceContext*    pd3dImmediateContextOrg = *ppd3dImmediateContext;
    ID3D11Device*          pd3dDeviceOrg = *ppd3dDevice;

    ID3D11DeviceContext*    pd3dImmediateContextWrapped = NULL;
    ID3D11Device*          pd3dDeviceWrapped = NULL;

    ALVR_RESULT res = pAffinity->WrapDeviceD3D11(pd3dDeviceOrg, &pd3dDeviceWrapped, &pd3dImmediateContextWrapped, &m_pLiquidVrDeviceContext);

	if (res == ALVR_OK)
	{
        *ppd3dImmediateContext = pd3dImmediateContextWrapped;
		pd3dImmediateContextOrg->Release(); // release original context- owned by wrapper now

        *ppd3dDevice = pd3dDeviceWrapped;
        pd3dDeviceOrg->Release(); // release original device - owned by wrapper now


		m_bUseAffinity = true;
        // test

        ID3D11Device*          pd3dDeviceWrappedTest =  NULL;
        ID3D11DeviceContext*    pd3dImmediateContextWrappedTest = NULL;

        pd3dImmediateContextWrapped->GetDevice(&pd3dDeviceWrappedTest);
        pd3dDeviceWrapped->GetImmediateContext(&pd3dImmediateContextWrappedTest);

        assert(pd3dDeviceWrappedTest == pd3dDeviceWrapped);
        assert(pd3dImmediateContextWrappedTest == pd3dImmediateContextWrapped);


        pd3dDeviceWrappedTest->Release();
        pd3dImmediateContextWrappedTest->Release();
        SetUseAffinity(bUseAffinity);
        // end of test
	}
	return S_OK;
}
//-------------------------------------------------------------------------------------------------
HRESULT     MGPUAffinity::CreateSyncObjects(ALVRDeviceExD3D11* pLiquidVrDevice)
{
    m_pLiquidVrDevice = pLiquidVrDevice;
    if (m_pLiquidVrDevice != NULL)
    {
        pLiquidVrDevice->CreateGpuSemaphore(&m_pRenderComplete0);
        pLiquidVrDevice->CreateGpuSemaphore(&m_pRenderComplete1);
        pLiquidVrDevice->CreateGpuSemaphore(&m_pTransferComplete0);
        pLiquidVrDevice->CreateGpuSemaphore(&m_pTransferComplete1);
    }
    return S_OK;
}
//-------------------------------------------------------------------------------------------------
HRESULT MGPUAffinity::Terminate()
{
    if (m_pRenderComplete0 != NULL)
    {
        m_pRenderComplete0->Release();
        m_pRenderComplete0 = NULL;
    }
    if (m_pRenderComplete1 != NULL)
    {
        m_pRenderComplete1->Release();
        m_pRenderComplete1 = NULL;
    }
    if (m_pTransferComplete0 != NULL)
    {
        m_pTransferComplete0->Release();
        m_pTransferComplete0 = NULL;
    }
    if (m_pTransferComplete1 != NULL)
    {
        m_pTransferComplete1->Release();
        m_pTransferComplete1 = NULL;
    }
    if (m_pLiquidVrDeviceContext != NULL)
    {
        m_pLiquidVrDeviceContext->Release();
        m_pLiquidVrDeviceContext = NULL;
    }

    if (m_pRenderComplete0 != NULL)
    {
        m_pRenderComplete0->Release();
        m_pRenderComplete0 = NULL;
    }
    if (m_pRenderComplete1 != NULL)
    {
        m_pRenderComplete1->Release();
        m_pRenderComplete1 = NULL;
    }
    if (m_pTransferComplete0 != NULL)
    {
        m_pTransferComplete0->Release();
        m_pTransferComplete0 = NULL;
    }
    if (m_pTransferComplete1 != NULL)
    {
        m_pTransferComplete1->Release();
        m_pTransferComplete1 = NULL;
    }
    m_pLiquidVrDevice = NULL;
	return S_OK;
}
//-------------------------------------------------------------------------------------------------
HRESULT MGPUAffinity::SetRenderGpuMask(GpuMask mask)
{
	if (m_bUseAffinity && m_pLiquidVrDeviceContext != NULL)
	{
		m_pLiquidVrDeviceContext->SetGpuRenderAffinity(mask);
		m_eCurrentMask = mask;
	}
	return S_OK;
}
//-------------------------------------------------------------------------------------------------
HRESULT MGPUAffinity::SetupViewport(ID3D11DeviceContext* pd3dImmediateContext, GpuMask mask, float fbWidth, float fbHeight)
{
	// viewport
	D3D11_VIEWPORT viewport;

	bool bPortrait = fbHeight > fbWidth;
	//need exact knowledge of display left-right eye structure - this is substitution

	if (bPortrait)
	{
		switch (mask)
		{
		case GPUMASK_LEFT:
			viewport.TopLeftY = 0.0f;
			viewport.Height = fbHeight / 2.0f;
			break;
		case GPUMASK_RIGHT:
			viewport.TopLeftY = fbHeight / 2.0f;
			viewport.Height = fbHeight / 2.0f;
			break;
		case GPUMASK_BOTH:
			viewport.TopLeftY = 0.0f;
			viewport.Height = fbHeight;
			break;
		}
		viewport.TopLeftX = 0.0f;
		viewport.Width = fbWidth;
	}
	else
	{
		switch (mask)
		{
		case GPUMASK_LEFT:
			viewport.TopLeftX = 0.0f;
			viewport.Width = fbWidth / 2.0f;
			break;
		case GPUMASK_RIGHT:
			viewport.TopLeftX = fbWidth / 2.0f;
			viewport.Width = fbWidth / 2.0f;
			break;
		case GPUMASK_BOTH:
			viewport.TopLeftX = 0.0f;
			viewport.Width = fbWidth;
			break;
		}
		viewport.TopLeftY = 0.0f;
		viewport.Height = fbHeight;
	}
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	pd3dImmediateContext->RSSetViewports(1, &viewport);
	return S_OK;
}
//-------------------------------------------------------------------------------------------------
void MGPUAffinity::SetUseAffinity(bool bUse)
{
	if (!bUse)
	{
		SetRenderGpuMask(GPUMASK_LEFT);
	}
	m_bUseAffinity = bUse;
}
//-------------------------------------------------------------------------------------------------
TransferSyncMode	MGPUAffinity::GetSyncType()
{
    return m_eTransferSyncMode;
}
//-------------------------------------------------------------------------------------------------
void			    MGPUAffinity::SetSyncType(TransferSyncMode eMode)
{
    m_eTransferSyncMode = eMode;
}
//-------------------------------------------------------------------------------------------------
bool MGPUAffinity::GetUseAffinity()
{
	return m_bUseAffinity && m_pLiquidVrDeviceContext != NULL;
}
//-------------------------------------------------------------------------------------------------
GpuMask MGPUAffinity::GetCurrentMask()
{
	return m_eCurrentMask;
}
//-------------------------------------------------------------------------------------------------
void        MGPUAffinity::MarkResourceAsInstanced(ID3D11Resource* pResource)
{
    if (m_pLiquidVrDeviceContext != NULL)
    {
        m_pLiquidVrDeviceContext->MarkResourceAsInstanced(pResource);
    }
}
//-------------------------------------------------------------------------------------------------
void MGPUAffinity::TransferRightEye(ID3D11DeviceContext* pd3dImmediateContext, float fbWidth, float fbHeight)
{
    ALVR_RESULT res = ALVR_OK;
	if (m_bUseAffinity && m_pLiquidVrDeviceContext != NULL)
	{
		bool bPortrait = fbHeight > fbWidth;
		ID3D11RenderTargetView* pRTV = NULL;
		pd3dImmediateContext->OMGetRenderTargets(1, &pRTV, NULL);

		// Transfer the resource from GPU 1 -> 0
		ID3D11Resource* pRT = NULL;
		D3D11_RECT halfScreen;
		if (bPortrait)
		{
			halfScreen.left = 0;
			halfScreen.top = (LONG)(fbHeight / 2.0f);
			halfScreen.right = (LONG)(fbWidth);
			halfScreen.bottom = (LONG)(fbHeight);
		}
		else
		{
			halfScreen.left = (LONG)(fbWidth / 2.0f);
			halfScreen.top = 0;
			halfScreen.right = (LONG)(fbWidth);
			halfScreen.bottom = (LONG)(fbHeight);
		}
		pRTV->GetResource(&pRT);
        switch (m_eTransferSyncMode)
        {
        case TRANSFER_SYNC_AUTOMATIC:
            m_pLiquidVrDeviceContext->TransferResource(pRT, pRT, 1, 0, 0, 0, &halfScreen, &halfScreen);
            break;
        case TRANSFER_SYNC_MANUAL:
        {
            ALVRGpuSemaphore*      pRenderComplete = m_bFrameLoop ? m_pRenderComplete0 : m_pRenderComplete1;
            ALVRGpuSemaphore*      pTransferComplete = m_bFrameLoop ? m_pTransferComplete0 : m_pTransferComplete1;

            m_bFrameLoop = !m_bFrameLoop;

            res = m_pLiquidVrDevice->QueueSemaphoreSignal(ALVR_GPU_ENGINE_3D, 0, pRenderComplete);
            res = m_pLiquidVrDevice->QueueSemaphoreWait(ALVR_GPU_ENGINE_3D, 1, pRenderComplete);

            m_pLiquidVrDeviceContext->TransferResourceEx(pRT, pRT, 1, 0, 0, 0, &halfScreen, &halfScreen, ALVR_GPU_ENGINE_3D, false);

            res = m_pLiquidVrDevice->QueueSemaphoreSignal(ALVR_GPU_ENGINE_3D, 1, pTransferComplete);
            res = m_pLiquidVrDevice->QueueSemaphoreWait(ALVR_GPU_ENGINE_3D, 0, pTransferComplete);
        }
            break;
        case TRANSFER_SYNC_NONE:
            m_pLiquidVrDeviceContext->TransferResourceEx(pRT, pRT, 1, 0, 0, 0, &halfScreen, &halfScreen, ALVR_GPU_ENGINE_3D, false);
            break;
        }
		pRT->Release();
		pRTV->Release();
	}
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
