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
#include "..\inc\ClearView.h"
#include "..\inc\ApplicationContext.h"

struct CB_PS_COLOR
{
	XMFLOAT4 m_vColor;
};
// Vertex structure for rendering full screen quads 
struct QuadVertex
{
	XMFLOAT3 v3Pos;
	XMFLOAT2 v2TexCoord;
};

//-------------------------------------------------------------------------------------------------
ClearView::ClearView() :
	m_pContext(NULL),
	m_pcbPSColor(NULL),
	m_pColorVertexLayout(NULL),
	m_pColorVS(NULL),
	m_pColorPS(NULL),
	m_pQuadVertexBuffer(NULL)
{
}
//-------------------------------------------------------------------------------------------------
ClearView::~ClearView()
{
	Terminate();
}
//-------------------------------------------------------------------------------------------------
HRESULT ClearView::Init(ID3D11Device* pd3dDevice, ApplicationContext *pContext)
{
	HRESULT hr = S_OK;
	m_pContext = pContext;

	// Define screen quad vertex layout
	D3D11_INPUT_ELEMENT_DESC QuadVertexLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	// Color VS
	m_pContext->m_ShaderCache.AddShader((ID3D11DeviceChild**)&m_pColorVS, AMD::ShaderCache::SHADER_TYPE_VERTEX, L"vs_5_0", L"VSColor",
		L"Shaders\\Source\\Color.hlsl", 0, NULL, &m_pColorVertexLayout, (D3D11_INPUT_ELEMENT_DESC*)QuadVertexLayout, ARRAYSIZE(QuadVertexLayout));
	// Color PS
	m_pContext->m_ShaderCache.AddShader((ID3D11DeviceChild**)&m_pColorPS, AMD::ShaderCache::SHADER_TYPE_PIXEL, L"ps_5_0", L"PSColor",
		L"Shaders\\Source\\Color.hlsl", 0, NULL, NULL, NULL, 0);

	D3D11_BUFFER_DESC Desc;

	// Utility
	Desc.Usage = D3D11_USAGE_DYNAMIC;
	Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Desc.MiscFlags = 0;

	Desc.ByteWidth = sizeof(CB_PS_COLOR);
	V_RETURN(pd3dDevice->CreateBuffer(&Desc, nullptr, &m_pcbPSColor));
	DXUT_SetDebugName(m_pcbPSColor, "CB_PS_COLOR");

	// Fill out a unit quad
	QuadVertex QuadVertices[6];
	QuadVertices[0].v3Pos = XMFLOAT3(-1.0f, -1.0f, 0.5f);
	QuadVertices[0].v2TexCoord = XMFLOAT2(0.0f, 1.0f);
	QuadVertices[1].v3Pos = XMFLOAT3(-1.0f, 1.0f, 0.5f);
	QuadVertices[1].v2TexCoord = XMFLOAT2(0.0f, 0.0f);
	QuadVertices[2].v3Pos = XMFLOAT3(1.0f, -1.0f, 0.5f);
	QuadVertices[2].v2TexCoord = XMFLOAT2(1.0f, 1.0f);
	QuadVertices[3].v3Pos = XMFLOAT3(-1.0f, 1.0f, 0.5f);
	QuadVertices[3].v2TexCoord = XMFLOAT2(0.0f, 0.0f);
	QuadVertices[4].v3Pos = XMFLOAT3(1.0f, 1.0f, 0.5f);
	QuadVertices[4].v2TexCoord = XMFLOAT2(1.0f, 0.0f);
	QuadVertices[5].v3Pos = XMFLOAT3(1.0f, -1.0f, 0.5f);
	QuadVertices[5].v2TexCoord = XMFLOAT2(1.0f, 1.0f);

	// Create the vertex buffer
	D3D11_BUFFER_DESC BD;
	BD.Usage = D3D11_USAGE_DYNAMIC;
	BD.ByteWidth = sizeof(QuadVertex) * 6;
	BD.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	BD.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	BD.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA InitData;
	InitData.pSysMem = QuadVertices;
	hr = pd3dDevice->CreateBuffer(&BD, &InitData, &m_pQuadVertexBuffer);
	return S_OK;
}
//-------------------------------------------------------------------------------------------------
HRESULT ClearView::Terminate()
{
	SAFE_RELEASE(m_pcbPSColor);
	SAFE_RELEASE(m_pColorVertexLayout);
	SAFE_RELEASE(m_pColorVS);
	SAFE_RELEASE(m_pColorPS);
	SAFE_RELEASE(m_pQuadVertexBuffer);

	m_pContext = NULL;
	return S_OK;
}
//-------------------------------------------------------------------------------------------------
HRESULT ClearView::Render(ID3D11DeviceContext* pd3dImmediateContext, XMVECTORF32 &color)
{
	// setup color
	HRESULT hr;
	D3D11_MAPPED_SUBRESOURCE MappedResource;
	V(pd3dImmediateContext->Map(m_pcbPSColor, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
	auto pPSColor = reinterpret_cast<CB_PS_COLOR*>(MappedResource.pData);
	XMStoreFloat4(&pPSColor->m_vColor, color);
	pd3dImmediateContext->Unmap(m_pcbPSColor, 0);

	pd3dImmediateContext->PSSetConstantBuffers(0, 1, &m_pcbPSColor);

	// Useful common locals
	UINT Stride = sizeof(QuadVertex);
	UINT Offset = 0;

	// Set common samplers
	ID3D11SamplerState* ppSamplerStates[2] = { m_pContext->m_pSamplePoint, m_pContext->m_pSampleLinear };
	pd3dImmediateContext->PSSetSamplers(0, 2, ppSamplerStates);

	// Set input layout and screen quad VS
	pd3dImmediateContext->IASetInputLayout(m_pColorVertexLayout);
	pd3dImmediateContext->IASetVertexBuffers(0, 1, &m_pQuadVertexBuffer, &Stride, &Offset);
	pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Set shaders
	pd3dImmediateContext->VSSetShader(m_pColorVS, NULL, 0);
	pd3dImmediateContext->PSSetShader(m_pColorPS, NULL, 0);

	// Do the draw
	pd3dImmediateContext->Draw(6, 0);

	// reset shaders
	pd3dImmediateContext->VSSetShader(NULL, NULL, 0);
	pd3dImmediateContext->PSSetShader(NULL, NULL, 0);

	return S_OK;
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
