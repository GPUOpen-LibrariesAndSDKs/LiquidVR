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
#include "..\inc\Background.h"
#include "AMD_SDK.h"

// Constant buffer layout for transfering data to the utility HLSL functions
struct CB_UTILITY
{
	XMMATRIX f4x4World;					// World matrix for object
	XMMATRIX f4x4View;					// View matrix
	XMMATRIX f4x4WorldViewProjection;	// World * View * Projection matrix  
	XMVECTOR fEyePoint;					// Eye	
	float f2RTSize[2];					// Render target size
	float f2Padding[2];					// Padding
	XMVECTOR f4LightDir;                // Light direction vector
};

//-------------------------------------------------------------------------------------------------
Background::Background() :
	m_pContext(NULL),
	m_pSceneVS(NULL),
	m_pScenePS(NULL),
	m_pSceneVertexLayout(NULL),
	m_pcbUtility(NULL),
    m_bDrawWithAlpha(false)
{

}
//-------------------------------------------------------------------------------------------------
Background::~Background()
{
	Terminate();
}
//-------------------------------------------------------------------------------------------------
HRESULT Background::Init(ID3D11Device* pd3dDevice, ApplicationContext *pContext)
{
	HRESULT hr = S_OK;

	m_pContext = pContext;

	V_RETURN(m_SceneMesh.Create(pd3dDevice, L"softparticles\\tankscene.sdkmesh"));

//    V_RETURN(m_SceneMesh.Create(pd3dDevice, L"test\\test1.sdkmesh"));
//    m_bDrawWithAlpha = true;
    
	// Define our scene vertex data layout
	const D3D11_INPUT_ELEMENT_DESC SceneLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	// Main scene VS
	m_pContext->m_ShaderCache.AddShader((ID3D11DeviceChild**)&m_pSceneVS, AMD::ShaderCache::SHADER_TYPE_VERTEX, L"vs_5_0", L"VS_RenderScene",
		L"Shaders\\Source\\Utility.hlsl", 0, NULL, &m_pSceneVertexLayout, (D3D11_INPUT_ELEMENT_DESC*)SceneLayout, ARRAYSIZE(SceneLayout));

	// Main scene PS
	m_pContext->m_ShaderCache.AddShader((ID3D11DeviceChild**)&m_pScenePS, AMD::ShaderCache::SHADER_TYPE_PIXEL, L"ps_5_0", L"PS_RenderScene",
		L"Shaders\\Source\\Utility.hlsl", 0, NULL, NULL, NULL, 0);

	D3D11_BUFFER_DESC Desc;

	// Utility
	Desc.Usage = D3D11_USAGE_DYNAMIC;
	Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Desc.MiscFlags = 0;
	Desc.ByteWidth = sizeof(CB_UTILITY);
	V_RETURN(pd3dDevice->CreateBuffer(&Desc, NULL, &m_pcbUtility));

    m_pContext->m_Affinity.MarkResourceAsInstanced(m_pcbUtility);

	return S_OK;
}
//-------------------------------------------------------------------------------------------------
HRESULT Background::Terminate()
{
	m_SceneMesh.Destroy();

	SAFE_RELEASE(m_pSceneVertexLayout);
	SAFE_RELEASE(m_pSceneVS);
	SAFE_RELEASE(m_pScenePS);
	SAFE_RELEASE(m_pcbUtility);

	m_pContext = NULL;
	return S_OK;
}
//-------------------------------------------------------------------------------------------------
HRESULT Background::Render(ID3D11DeviceContext* pd3dImmediateContext, float fbWidth, float fbHeight)
{
	ID3D11SamplerState* ppSamplerStates[2] = { m_pContext->m_pSamplePoint, m_pContext->m_pSampleLinear };

	bool bPortrait = fbHeight > fbWidth;

	m_pContext->m_Affinity.SetRenderGpuMask(GPUMASK_BOTH);
	// Set the shaders

	pd3dImmediateContext->PSSetSamplers(0, 2, ppSamplerStates);

	pd3dImmediateContext->VSSetShader(m_pSceneVS, NULL, 0);
	pd3dImmediateContext->PSSetShader(m_pScenePS, NULL, 0);


	// Switch off alpha blending
    float BlendFactor[4] ={1.0f, 1.0f, 1.0f, 1.0f};
    if(m_bDrawWithAlpha)
    {
        pd3dImmediateContext->OMSetBlendState(m_pContext->m_pAlphaState, BlendFactor, 0xffffffff);
        pd3dImmediateContext->OMSetDepthStencilState(m_pContext->m_pDepthStateAlpha, 0);
    }
    else
    {
        pd3dImmediateContext->OMSetBlendState(m_pContext->m_pOpaqueState, BlendFactor, 0xffffffff);
        pd3dImmediateContext->OMSetDepthStencilState(m_pContext->m_pDepthStateOpaque, 0);
    }

	// Set the vertex buffer format
	pd3dImmediateContext->IASetInputLayout(m_pSceneVertexLayout);

	m_pContext->m_Affinity.SetRenderGpuMask(GPUMASK_LEFT);
	// Render the scene mesh 
	m_pContext->m_Affinity.SetupViewport(pd3dImmediateContext, GPUMASK_LEFT, fbWidth, fbHeight);
	SetupEye(pd3dImmediateContext, GPUMASK_LEFT, bPortrait);

	pd3dImmediateContext->VSSetConstantBuffers(0, 1, &m_pcbUtility);
	pd3dImmediateContext->PSSetConstantBuffers(0, 1, &m_pcbUtility);

	if (!m_pContext->m_Affinity.GetUseAffinity())
	{
		m_SceneMesh.Render(pd3dImmediateContext, 2, 3);
	}
	m_pContext->m_Affinity.SetRenderGpuMask(GPUMASK_RIGHT);
	m_pContext->m_Affinity.SetupViewport(pd3dImmediateContext, GPUMASK_RIGHT, fbWidth, fbHeight);
	SetupEye(pd3dImmediateContext, GPUMASK_RIGHT, bPortrait);
	pd3dImmediateContext->VSSetConstantBuffers(0, 1, &m_pcbUtility);
	pd3dImmediateContext->PSSetConstantBuffers(0, 1, &m_pcbUtility);

	m_pContext->m_Affinity.SetRenderGpuMask(GPUMASK_BOTH);
	m_SceneMesh.Render(pd3dImmediateContext, 2, 3);
	return S_OK;
}
//-------------------------------------------------------------------------------------------------
void Background::SetupEye(ID3D11DeviceContext* pd3dImmediateContext, GpuMask mask, bool bPortrait)
{
	// Get the projection & view matrix from the camera class
	XMMATRIX mWorld = XMMatrixIdentity();
	XMMATRIX mView = m_pContext->m_Camera.GetViewMatrix();
	XMMATRIX mProj = m_pContext->m_Camera.GetProjMatrix();


	// Make offsets for left and right eye
	XMMATRIX eyeView;
	eyeView = XMMatrixIdentity();
	
	switch (mask)
	{
	case GPUMASK_LEFT:
		eyeView = XMMatrixTranslation(-0.01f, 0, 0);
		break;
	case GPUMASK_RIGHT:
		eyeView = XMMatrixTranslation(0.01f, 0, 0);
		break;
	case GPUMASK_BOTH:
		eyeView = XMMatrixIdentity();
		break;
	}
	
	mWorld = mWorld * (mView * eyeView);

	if (bPortrait)
	{
		mWorld = mWorld *XMMatrixRotationZ(XM_PIDIV2);
	}

	XMMATRIX worldViewProjection = mWorld * mProj;

	// VS Per object 
	CB_UTILITY UTILITY_CB;
	UTILITY_CB.f4x4World = XMMatrixTranspose(mWorld);
	UTILITY_CB.f4x4View = XMMatrixTranspose(mView);
	UTILITY_CB.f4x4WorldViewProjection = XMMatrixTranspose(worldViewProjection);
	UTILITY_CB.fEyePoint = m_pContext->m_Camera.GetEyePt();
	UTILITY_CB.f2RTSize[0] = (float)DXUTGetDXGIBackBufferSurfaceDesc()->Width;
	UTILITY_CB.f2RTSize[1] = (float)DXUTGetDXGIBackBufferSurfaceDesc()->Height;
	UTILITY_CB.f4LightDir = m_pContext->m_Light.GetLightDirection();
	D3D11_MAPPED_SUBRESOURCE MappedResource;
    pd3dImmediateContext->Map(m_pcbUtility, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
	memcpy(MappedResource.pData, &UTILITY_CB, sizeof(CB_UTILITY));
    pd3dImmediateContext->Unmap(m_pcbUtility, 0);
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
