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
#pragma once

#include <d3d11.h>
#include <DirectXMath.h>

using namespace DirectX;

class ApplicationContext;
class ClearView
{
public:
	ClearView();
	virtual ~ClearView();

	HRESULT Init(ID3D11Device* pd3dDevice, ApplicationContext *pContext);
	HRESULT Terminate();
	HRESULT Render(ID3D11DeviceContext* pd3dImmediateContext, XMVECTORF32 &color);
protected:
	ApplicationContext*					m_pContext;
	ID3D11Buffer*						m_pcbPSColor;
	ID3D11InputLayout*					m_pColorVertexLayout;
	ID3D11VertexShader*					m_pColorVS;
	ID3D11PixelShader*					m_pColorPS;
	ID3D11Buffer*						m_pQuadVertexBuffer;

};