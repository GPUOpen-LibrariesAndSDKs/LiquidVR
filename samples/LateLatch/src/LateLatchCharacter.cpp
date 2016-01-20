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

#include "..\inc\LateLatchCharacter.h"
#include "AMD_SDK.h"

// must match the same in shader code
UINT LateLatchCharacter::m_uiNumberOfLateLatchBuffers = 512; // close to the limit

UINT LateLatchCharacter::m_iCBVSSIndexLatch = 2;

//-------------------------------------------------------------------------------------------------
LateLatchCharacter::LateLatchCharacter() :
m_pConstantBufferLeft(NULL),
m_pConstantBufferRight(NULL),
m_bLateLatch(true),
m_iUpdatesPerFrame(0),
m_bLateLatchLeftEyeOnly(true)
{
#ifdef _DEBUG
    m_iMaxDraws = 10000; // add more
    m_iTodoDraws = 10000;
#else
	m_iMaxDraws = 70000; // add more
	m_iTodoDraws = 70000;
#endif
	m_bShift = false;
	m_bRotate = false;
}
//-------------------------------------------------------------------------------------------------
LateLatchCharacter::~LateLatchCharacter()
{

}
//-------------------------------------------------------------------------------------------------
WCHAR* LateLatchCharacter::GetVShaderName()
{
	return L"Shaders\\Source\\SimpleLateLatch_VS.hlsl";
}
//-------------------------------------------------------------------------------------------------
HRESULT LateLatchCharacter::Terminate()
{
    if (m_pConstantBufferLeft)
    {
        m_pConstantBufferLeft->Release();
    }
    if (m_pConstantBufferRight)
    {
        m_pConstantBufferRight->Release();
    }
	return Character::Terminate();
}
//-------------------------------------------------------------------------------------------------
HRESULT LateLatchCharacter::AllocVShaderObjects(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext)
{
	HRESULT hr = S_OK;
	ALVR_RESULT res = ALVR_OK;
	res = m_pContext->m_pLiquidVrDevice->CreateLateLatchConstantBufferD3D11(sizeof(CB_VS_PER_OBJECT), m_uiNumberOfLateLatchBuffers, 0, &m_pConstantBufferLeft);
	V_RETURN(hr = res == ALVR_OK ? S_OK : E_FAIL);
    res = m_pContext->m_pLiquidVrDevice->CreateLateLatchConstantBufferD3D11(sizeof(CB_VS_PER_OBJECT), m_uiNumberOfLateLatchBuffers, 0, &m_pConstantBufferRight);
	V_RETURN(hr = res == ALVR_OK ? S_OK : E_FAIL);
	return S_OK;
}
//-------------------------------------------------------------------------------------------------
HRESULT LateLatchCharacter::UpdateVShaderObjects(ID3D11DeviceContext* pd3dImmediateContext, XMMATRIX &viewProj, XMMATRIX &mWorld, GpuMask mask)
{
    ALVRLateLatchConstantBufferD3D11 *pConstantBuffer = mask != GPUMASK_RIGHT ? m_pConstantBufferLeft : m_pConstantBufferRight;
	CB_VS_PER_OBJECT VSPerObject;
	XMStoreFloat4x4(&VSPerObject.m_ViewProj, viewProj);
	XMStoreFloat4x4(&VSPerObject.m_World, mWorld);
    return pConstantBuffer->Update((uint8_t*)&VSPerObject, 0, sizeof(VSPerObject));
}
//-------------------------------------------------------------------------------------------------
HRESULT LateLatchCharacter::Render(ID3D11DeviceContext* pd3dImmediateContext, float fbWidth, float fbHeight)
{
	m_iUpdatesPerFrame = 0;
	GpuMask mask = m_pContext->m_Affinity.GetCurrentMask();
	m_pContext->m_Affinity.SetRenderGpuMask(GPUMASK_LEFT);
	m_pConstantBufferLeft->QueueLatch();
	m_pContext->m_Affinity.SetRenderGpuMask(GPUMASK_RIGHT);
    m_pConstantBufferRight->QueueLatch();
	m_pContext->m_Affinity.SetRenderGpuMask(mask);
	return Character::Render(pd3dImmediateContext, fbWidth, fbHeight);
}
//-------------------------------------------------------------------------------------------------
HRESULT LateLatchCharacter::SetPosition(float x, float y)
{
	if (m_fPosX == x && m_fPosY == y)
	{
		return S_OK;
	}
	Character::SetPosition(x, y);

	if (m_bLateLatch)
	{
		bool bPortrait = m_pContext->m_fbHeight > m_pContext->m_fbWidth;

		// this is called from a thread - pd3dImmediateContext is NULL - not needed for late Latching;
		SetupEye(NULL, GPUMASK_LEFT, bPortrait);
		if (!m_bLateLatchLeftEyeOnly)
		{
			SetupEye(NULL, GPUMASK_RIGHT, bPortrait);
		}
		m_iUpdatesPerFrame++;
	}
	return S_OK;
}
//-------------------------------------------------------------------------------------------------
HRESULT LateLatchCharacter::SetupVS(ID3D11DeviceContext* pd3dImmediateContext, GpuMask mask)
{
    ALVRLateLatchConstantBufferD3D11 *pConstantBuffer = mask != GPUMASK_RIGHT ? m_pConstantBufferLeft : m_pConstantBufferRight;
	ID3D11Buffer* pcbVSPerObject = pConstantBuffer->GetDataD3D11();
    ID3D11Buffer* pcbVSIndex = pConstantBuffer->GetIndexD3D11();
	pd3dImmediateContext->VSSetConstantBuffers(0, 1, &pcbVSPerObject);
    pd3dImmediateContext->VSSetConstantBuffers(2, 1, &m_pcbPSPerFrame);
	pd3dImmediateContext->VSSetConstantBuffers(3, 1, &pcbVSIndex);
	return S_OK;
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
