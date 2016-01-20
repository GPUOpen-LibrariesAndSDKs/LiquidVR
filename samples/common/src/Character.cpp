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
#include "..\inc\Character.h"
#include "AMD_SDK.h"


struct CB_VS_POSITION
{
	XMFLOAT4X4 m_Position;
};
static UINT                        g_iCBVSPosition = 1;

struct CB_PS_PER_OBJECT
{
	XMFLOAT4 m_vObjectColor;
};
UINT								g_iCBPSPerObjectBind = 0;
struct CB_PS_PER_FRAME
{
	XMFLOAT4 g_vLightDir; // float3: g_vLightDir + float: g_fAmbient
};
UINT								g_iCBPSPerFrameBind = 1;

UINT Character::m_iCBVSPerObjectBind = 0;

//-------------------------------------------------------------------------------------------------
Character::Character() :
m_pContext(NULL),
m_pSimpleVS(NULL),
m_pSimplePS(NULL),
m_pVertexLayout(NULL),
m_pcbVSPerObject(NULL),
m_pcbPSPerObject(NULL),
m_pcbPSPerFrame(NULL),
m_bShift(true),
m_bAlpha(false),
m_bClear(false),
m_bRotate(true),
m_pcbVSPosition(NULL),
m_iPositionCount(0),
m_iPositionAllocated(0),
m_iMinDraws(1),
m_iMaxDraws(10000),
m_iTodoDraws(1000),
m_dwTime(0),
m_fTime(0),
m_fPosX(0),
m_fPosY(0),
m_bUseTimers(true)
{

}
//-------------------------------------------------------------------------------------------------
Character::~Character()
{
	Terminate();
}
//-------------------------------------------------------------------------------------------------
WCHAR* Character::GetVShaderName()
{
	return L"Shaders\\Source\\Simple_VS.hlsl";
}
//-------------------------------------------------------------------------------------------------
HRESULT Character::Init(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, ApplicationContext *pContext)
{
	HRESULT hr = S_OK;

	m_pContext = pContext;

//	V_RETURN(m_Mesh.Create(pd3dDevice, L"Foblin\\Foblin.sdkmesh"));
	V_RETURN(m_Mesh.Create(pd3dDevice, L"Froblin\\Froblin.sdkmesh"));

	// Create our character vertex input layout
	const D3D11_INPUT_ELEMENT_DESC CharacterLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	// Simple Character VS
	m_pContext->m_ShaderCache.AddShader((ID3D11DeviceChild**)&m_pSimpleVS, AMD::ShaderCache::SHADER_TYPE_VERTEX, L"vs_5_0", L"VSMain",
		GetVShaderName(), 0, NULL, &m_pVertexLayout, (D3D11_INPUT_ELEMENT_DESC*)CharacterLayout, ARRAYSIZE(CharacterLayout));


	// Simple Character PS
	m_pContext->m_ShaderCache.AddShader((ID3D11DeviceChild**)&m_pSimplePS, AMD::ShaderCache::SHADER_TYPE_PIXEL, L"ps_5_0", L"PSMain",
		L"Shaders\\Source\\Simple_PS.hlsl", 0, NULL, NULL, NULL, 0);

	// PS object
	D3D11_BUFFER_DESC Desc;

	Desc.Usage = D3D11_USAGE_DYNAMIC;
	Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Desc.MiscFlags = 0;

	Desc.ByteWidth = sizeof(CB_PS_PER_OBJECT);
	V_RETURN(pd3dDevice->CreateBuffer(&Desc, nullptr, &m_pcbPSPerObject));
	DXUT_SetDebugName(m_pcbPSPerObject, "CB_PS_PER_OBJECT");
    m_pContext->m_Affinity.MarkResourceAsInstanced(m_pcbPSPerObject);

	Desc.ByteWidth = sizeof(CB_PS_PER_FRAME);
	V_RETURN(pd3dDevice->CreateBuffer(&Desc, nullptr, &m_pcbPSPerFrame));
    DXUT_SetDebugName(m_pcbPSPerFrame, "CB_PS_PER_OBJECT");
    m_pContext->m_Affinity.MarkResourceAsInstanced(m_pcbPSPerFrame);

	return AllocVShaderObjects(pd3dDevice, pd3dImmediateContext);
}
//-------------------------------------------------------------------------------------------------
HRESULT Character::AllocVShaderObjects(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext)
{
	pd3dImmediateContext;

	// VS object
	HRESULT hr = S_OK;
	D3D11_BUFFER_DESC Desc;

	Desc.Usage = D3D11_USAGE_DYNAMIC;
	Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Desc.MiscFlags = 0;

	Desc.ByteWidth = sizeof(CB_VS_PER_OBJECT);
	V_RETURN(pd3dDevice->CreateBuffer(&Desc, nullptr, &m_pcbVSPerObject));
	DXUT_SetDebugName(m_pcbVSPerObject, "CB_VS_PER_OBJECT_LEFT");
    m_pContext->m_Affinity.MarkResourceAsInstanced(m_pcbVSPerObject);
	return S_OK;
}
//-------------------------------------------------------------------------------------------------
HRESULT Character::Terminate()
{
	m_Mesh.Destroy();

	SAFE_RELEASE(m_pVertexLayout);
	SAFE_RELEASE(m_pSimpleVS);
	SAFE_RELEASE(m_pSimplePS);
	SAFE_RELEASE(m_pcbVSPerObject);
	SAFE_RELEASE(m_pcbPSPerObject);
	SAFE_RELEASE(m_pcbPSPerFrame);

	ClearPositionBuffers();
	m_fPosX = 0;
	m_fPosY = 0;

	return S_OK;
}
//-------------------------------------------------------------------------------------------------
HRESULT Character::Render(ID3D11DeviceContext* pd3dImmediateContext, float fbWidth, float fbHeight)
{
	if (m_bUseTimers)
	{
		TIMER_Begin(0, L"FoblinMesh");
	}
	m_pContext->m_Affinity.SetRenderGpuMask(GPUMASK_BOTH);

	ID3D11Device* pd3dDevice = NULL;
	pd3dImmediateContext->GetDevice(&pd3dDevice);
	UpdatePositionBuffers(pd3dDevice, pd3dImmediateContext);
	pd3dDevice->Release();

	// Clear the render target and depth stencil

	bool bPortrait = fbHeight > fbWidth;

    // Switch on alpha blending
    if(m_bAlpha)
	{
		float BlendFactor[4] = { 0.3f, 0.3f, 0.3f, 0.3f };
		pd3dImmediateContext->OMSetBlendState(m_pContext->m_pAlphaState, BlendFactor, 0xffffffff);
    }
    else
    {
        float BlendFactor[4] ={1.0f, 1.0f, 1.0f, 1.0f};
        pd3dImmediateContext->OMSetBlendState(m_pContext->m_pOpaqueState, BlendFactor, 0xffffffff);
    }
    pd3dImmediateContext->OMSetDepthStencilState(m_pContext->m_pDepthStateOpaque, 0);

	// Set our clear colors such that it is easy to tell which GPU an image came from
	if (m_bClear)
	{
		XMVECTORF32 Crimson = { 0.862745166f, 0.078431375f, 0.235294133f, 0.250000000f };
		XMVECTORF32 MidnightBlue = { 0.098039225f, 0.098039225f, 0.439215720f, 0.250000000f };

		m_pContext->m_Affinity.SetRenderGpuMask(GPUMASK_LEFT);
		m_pContext->m_Affinity.SetupViewport(pd3dImmediateContext, GPUMASK_LEFT, fbWidth, fbHeight);
		m_pContext->m_ClearView.Render(pd3dImmediateContext, MidnightBlue);

		m_pContext->m_Affinity.SetRenderGpuMask(GPUMASK_RIGHT);
		m_pContext->m_Affinity.SetupViewport(pd3dImmediateContext, GPUMASK_RIGHT, fbWidth, fbHeight);

		m_pContext->m_ClearView.Render(pd3dImmediateContext, Crimson);
	}
	m_pContext->m_Affinity.SetRenderGpuMask(GPUMASK_BOTH);
	auto pDSV = DXUTGetD3D11DepthStencilView();
	if (pDSV != NULL)
	{
		pd3dImmediateContext->ClearDepthStencilView(pDSV, D3D11_CLEAR_DEPTH, 1.0, 0);
	}

	//Get the mesh
	//IA setup
	pd3dImmediateContext->IASetInputLayout(m_pVertexLayout);
	UINT Strides[1];
	UINT Offsets[1];
	ID3D11Buffer* pVB[1];
	pVB[0] = m_Mesh.GetVB11(0, 0);
	Strides[0] = (UINT)m_Mesh.GetVertexStride(0, 0);
	Offsets[0] = 0;
	pd3dImmediateContext->IASetVertexBuffers(0, 1, pVB, Strides, Offsets);
	pd3dImmediateContext->IASetIndexBuffer(m_Mesh.GetIB11(0), m_Mesh.GetIBFormat11(0), 0);

	// Set the shaders
	pd3dImmediateContext->VSSetShader(m_pSimpleVS, nullptr, 0);
	pd3dImmediateContext->PSSetShader(m_pSimplePS, nullptr, 0);

	m_pContext->m_Affinity.SetRenderGpuMask(GPUMASK_LEFT);

	m_pContext->m_Affinity.SetupViewport(pd3dImmediateContext, GPUMASK_LEFT, fbWidth, fbHeight);
	SetupEye(pd3dImmediateContext, GPUMASK_LEFT, bPortrait);
	SetupPS(pd3dImmediateContext, GPUMASK_LEFT);
	SetupVS(pd3dImmediateContext, GPUMASK_LEFT);


	if (m_bUseTimers)
	{
		TIMER_Begin(0, L"LeftEye");
	}
	{

		if (!m_pContext->m_Affinity.GetUseAffinity())
		{

			// Bind constant buffer for the left eye if SGPU
			// Draw left eye if SGPU
			DrawObjectsForView(pd3dImmediateContext);
		}
	}
	if (m_bUseTimers)
	{
		TIMER_End(); // Left Eye
	}

	m_pContext->m_Affinity.SetRenderGpuMask(GPUMASK_RIGHT);

	m_pContext->m_Affinity.SetupViewport(pd3dImmediateContext, GPUMASK_RIGHT, fbWidth, fbHeight);
	SetupEye(pd3dImmediateContext, GPUMASK_RIGHT, bPortrait);
	SetupPS(pd3dImmediateContext, GPUMASK_RIGHT);
	SetupVS(pd3dImmediateContext, GPUMASK_RIGHT);


	if (m_bUseTimers)
	{

		TIMER_Begin(0, L"RightEye");
	}
	{
		m_pContext->m_Affinity.SetRenderGpuMask(GPUMASK_BOTH);

		// Bind the above constant buffer for right eye if SGPU, or both eyes at once if MGPU

		// Draw right eye if SGPU, or both eyes at once if MGPU
		DrawObjectsForView(pd3dImmediateContext);
	}
	if (m_bUseTimers)
	{

		TIMER_End(); // Right Eye

		TIMER_End(); // FoblinMesh
	}
	return S_OK;
}
//-------------------------------------------------------------------------------------------------
void Character::DrawObjectsForView(ID3D11DeviceContext* pd3dImmediateContext)
{
	//Render
	pd3dImmediateContext->PSSetSamplers(0, 1, &m_pContext->m_pSampleLinear);

	for (int numDraws = 0; numDraws < m_iTodoDraws; ++numDraws)
	{

		pd3dImmediateContext->VSSetConstantBuffers(g_iCBVSPosition, 1, &m_pcbVSPosition[m_bShift ? numDraws : 0]);
		for (UINT subset = 0; subset < m_Mesh.GetNumSubsets(0); ++subset)
		{
			// Get the subset
			auto pSubset = m_Mesh.GetSubset(0, subset);

			auto PrimType = CDXUTSDKMesh::GetPrimitiveType11((SDKMESH_PRIMITIVE_TYPE)pSubset->PrimitiveType);
			pd3dImmediateContext->IASetPrimitiveTopology(PrimType);

			// Ignores most of the material information in them mesh to use only a simple shader
			auto pDiffuseRV = m_Mesh.GetMaterial(pSubset->MaterialID)->pDiffuseRV11;
			pd3dImmediateContext->PSSetShaderResources(0, 1, &pDiffuseRV);

			pd3dImmediateContext->DrawIndexed((UINT)pSubset->IndexCount, (UINT)pSubset->IndexStart, (UINT)pSubset->VertexStart);
		}
	}
}
//-------------------------------------------------------------------------------------------------
void Character::SetupEye(ID3D11DeviceContext* pd3dImmediateContext, GpuMask mask, bool bPortrait)
{
	// Get the projection & view matrix from the camera class
	XMMATRIX mWorld = XMMatrixIdentity();

	XMMATRIX mView = m_pContext->m_Camera.GetViewMatrix();
	XMMATRIX mProj = m_pContext->m_Camera.GetProjMatrix();

	XMMATRIX rotation = XMMatrixRotationY(m_fTime);
	mWorld = mWorld * rotation;


	// scale and rotate model
//	float scale = m_bShift ? 0.02f : 0.01f;
	float scale = m_bShift ? 0.002f : 0.01f;
	XMMATRIX mScale = XMMatrixScaling(scale, scale, scale);
	mWorld = mWorld * mScale;

	mWorld = mWorld * XMMatrixTranslation(0.0, 1.3f, 0.0f);

//	if (m_fPosX != 0 || m_fPosY != 0)
//	{
//		mWorld = mWorld * XMMatrixTranslation(m_fPosX, m_fPosY, 0);
//	}

	// Make offsets for left and right eye
	XMMATRIX eyeView;
	eyeView = XMMatrixIdentity();
	
    XMVECTORF32 color;
	
	switch (mask)
	{
	case GPUMASK_LEFT:
        eyeView = XMMatrixTranslation(1.0f, 0, 0);
        color = Colors::LightGreen;
		break;
	case GPUMASK_RIGHT:
		eyeView = XMMatrixTranslation(-1.0f, 0, 0);
        color = Colors::LightSalmon;
		break;
	case GPUMASK_BOTH:
		eyeView = XMMatrixIdentity();
        color = Colors::White;
        break;
	}
	
	XMMATRIX viewProjection = mView * eyeView * mProj;

	if (bPortrait)
	{
		viewProjection = viewProjection *XMMatrixRotationZ(XM_PIDIV2);
	}

	if (m_fPosX != 0 || m_fPosY != 0)
	{
		viewProjection = viewProjection * XMMatrixTranslation(m_fPosX, m_fPosY, 0);
	}

	// VS Per object
	XMMATRIX mtViewProjection = XMMatrixTranspose(viewProjection);
	XMMATRIX mtWorld = XMMatrixTranspose(mWorld);

	UpdateVShaderObjects(pd3dImmediateContext, mtViewProjection, mtWorld, mask);
    
    if (pd3dImmediateContext != NULL)
    {
        D3D11_MAPPED_SUBRESOURCE MappedResource;
        pd3dImmediateContext->Map(m_pcbPSPerObject, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
        CB_PS_PER_OBJECT *pPSPerObject = reinterpret_cast<CB_PS_PER_OBJECT*>(MappedResource.pData);
        XMStoreFloat4(&pPSPerObject->m_vObjectColor, color);
        pd3dImmediateContext->Unmap(m_pcbPSPerObject, 0);
    }
    
}
//-------------------------------------------------------------------------------------------------
HRESULT Character::UpdateVShaderObjects(ID3D11DeviceContext* pd3dImmediateContext, XMMATRIX &viewProj, XMMATRIX &mWorld, GpuMask mask)
{
    mask;
	D3D11_MAPPED_SUBRESOURCE MappedResource;
    pd3dImmediateContext->Map(m_pcbVSPerObject, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);

	auto pVSPerObject = reinterpret_cast<CB_VS_PER_OBJECT*>(MappedResource.pData);
	XMStoreFloat4x4(&pVSPerObject->m_ViewProj, viewProj);
	XMStoreFloat4x4(&pVSPerObject->m_World, mWorld);

    pd3dImmediateContext->Unmap(m_pcbVSPerObject, 0);

	return S_OK;
}
//-------------------------------------------------------------------------------------------------
void Character::SetupPS(ID3D11DeviceContext* pd3dImmediateContext, GpuMask mask)
{
    mask;
	// PS Per object
    pd3dImmediateContext->PSSetConstantBuffers(g_iCBPSPerObjectBind, 1, &m_pcbPSPerObject);


	D3D11_MAPPED_SUBRESOURCE MappedResource;
    pd3dImmediateContext->Map(m_pcbPSPerFrame, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
	XMVECTOR vLightDir = m_pContext->m_Light.GetLightDirection();
	auto pPerFrame = reinterpret_cast<CB_PS_PER_FRAME*>(MappedResource.pData);
	float fAmbient = 2.0f;
	XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&pPerFrame->g_vLightDir), vLightDir);
	pPerFrame->g_vLightDir.w = fAmbient;
    pd3dImmediateContext->Unmap(m_pcbPSPerFrame, 0);

    pd3dImmediateContext->PSSetConstantBuffers(g_iCBPSPerFrameBind, 1, &m_pcbPSPerFrame);
}
//-------------------------------------------------------------------------------------------------
HRESULT Character::SetupVS(ID3D11DeviceContext* pd3dImmediateContext, GpuMask mask)
{
    mask;
    pd3dImmediateContext->VSSetConstantBuffers(0, 1, &m_pcbVSPerObject);
    pd3dImmediateContext->VSSetConstantBuffers(2, 1, &m_pcbPSPerFrame);
	return S_OK;
}
//-------------------------------------------------------------------------------------------------
void Character::SetShift(bool bShift)
{
	m_bShift = bShift; 
	m_iPositionCount = 0;
}
//-------------------------------------------------------------------------------------------------
void Character::UpdatePositionBuffers(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext)
{
//	int toAllocate = 1;
	int toAllocate = m_iMaxDraws;
	int toUpdate = 0;
	if (m_bShift)
	{
		toAllocate = m_iMaxDraws;
		if (m_iTodoDraws != m_iPositionCount)
		{
			toUpdate = m_iTodoDraws;
		}
	}
	else
	{
		if (m_iPositionCount == 0)
		{
			toUpdate = 1;
		}
	}
	GpuMask mask = m_pContext->m_Affinity.GetCurrentMask();
    bool bUseAffinity = m_pContext->m_Affinity.GetUseAffinity();

	if (m_iPositionAllocated < toAllocate)

	{
		// need temporarely force BOTH
        m_pContext->m_Affinity.SetUseAffinity(true);
		m_pContext->m_Affinity.SetRenderGpuMask(GPUMASK_BOTH);
		// reallocate
		ID3D11Buffer**						pcbVSPosition = new ID3D11Buffer*[toAllocate];
		memcpy(pcbVSPosition, m_pcbVSPosition, m_iPositionCount * sizeof(*m_pcbVSPosition));
		delete[] m_pcbVSPosition;
		m_pcbVSPosition = pcbVSPosition;

		D3D11_BUFFER_DESC Desc;

		// Create buffers
		Desc.Usage = D3D11_USAGE_DYNAMIC;
		Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		Desc.MiscFlags = 0;

		Desc.ByteWidth = sizeof(CB_VS_POSITION);

		for (int i = m_iPositionAllocated; i < toAllocate; i++)
		{
			pd3dDevice->CreateBuffer(&Desc, nullptr, &m_pcbVSPosition[i]);
			DXUT_SetDebugName(m_pcbVSPosition[i], "CB_VS_PER_OBJECT_LEFT");
		}
		m_iPositionAllocated = toAllocate;
	}
	if (toUpdate)
	{
		// need temporarely force BOTH
        m_pContext->m_Affinity.SetUseAffinity(true);
        m_pContext->m_Affinity.SetRenderGpuMask(GPUMASK_BOTH);
		// refill buffers
		m_iPositionCount = toUpdate;

		int side = (int)pow((double)toUpdate,1./3.);

		for (int i = 0; i < toUpdate; i++)
		{
			FLOAT center = (FLOAT)(side / 2);

			int iReverse = toUpdate - i - 1;

			FLOAT x = (iReverse % side) - center;
			FLOAT y = ((iReverse / side) % side) - center;
			float z = (iReverse / side / side) - center;

			float shiftKoeff = 0.6f;
			XMMATRIX shift = XMMatrixTranspose(XMMatrixTranslation(x * shiftKoeff, y * shiftKoeff, z * shiftKoeff));

			D3D11_MAPPED_SUBRESOURCE MappedResource;
			pd3dImmediateContext->Map(m_pcbVSPosition[i], 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
			auto pVSPosition = reinterpret_cast<CB_VS_POSITION*>(MappedResource.pData);
			XMStoreFloat4x4(&pVSPosition->m_Position, shift);
			pd3dImmediateContext->Unmap(m_pcbVSPosition[i], 0);
		}
	}
    m_pContext->m_Affinity.SetRenderGpuMask(mask);
    m_pContext->m_Affinity.SetUseAffinity(bUseAffinity);
}
//-------------------------------------------------------------------------------------------------
void Character::ClearPositionBuffers()
{
	GpuMask mask = m_pContext->m_Affinity.GetCurrentMask();

	m_pContext->m_Affinity.SetRenderGpuMask(GPUMASK_BOTH);


	for (int i = 0; i < m_iPositionAllocated; i++)
	{
		m_pcbVSPosition[i]->Release();
	}
	delete[] m_pcbVSPosition;
	m_pcbVSPosition = NULL;
	m_iPositionAllocated = 0;
	m_iPositionCount = 0;

	m_pContext->m_Affinity.SetRenderGpuMask(mask);
}
//-------------------------------------------------------------------------------------------------
HRESULT Character::Time()
{
	// Update our time
	DWORD dwTimeCur = GetTickCount();
	if (m_dwTime == 0)
	{
		m_dwTime = dwTimeCur;
	}
	if (m_bRotate)
	{
		m_fTime = (dwTimeCur - m_dwTime) / 1000.0f;
	}
	return S_OK;
}
//-------------------------------------------------------------------------------------------------
HRESULT Character::SetPosition(float x, float y)
{
	m_fPosX = x;
	m_fPosY = y;

	return S_OK;
}
//-------------------------------------------------------------------------------------------------
