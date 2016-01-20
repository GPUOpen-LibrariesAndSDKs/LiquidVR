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
#include "MGPUAffinity.h"
#include "DXUTCamera.h"
#include "AMD_SDK.h"
#include "ClearView.h"

class ApplicationContext
{
public:
	ApplicationContext();
	virtual ~ApplicationContext();

	HRESULT Init(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext);
	HRESULT Terminate();
	void	OnResizedSwapChain(const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc);

public:
	AMD::ShaderCache			m_ShaderCache;
	MGPUAffinity				m_Affinity;
	float						m_fbWidth;
	float						m_fbHeight;
	CFirstPersonCamera          m_Camera;                   // A model viewing camera
	CDXUTDirectionWidget        m_Light;                    // Dynamic Light
	// Samplers
	ID3D11SamplerState*         m_pSamplePoint;
	ID3D11SamplerState*         m_pSampleLinear;
	// Blend states
	ID3D11BlendState*           m_pAlphaState;
	ID3D11BlendState*           m_pOpaqueState;

    ID3D11DepthStencilState*    m_pDepthStateAlpha;
    ID3D11DepthStencilState*    m_pDepthStateOpaque;

	ClearView					m_ClearView;

	ALVRFactory*			    m_pLiquidVRFactory;
    ALVRDeviceExD3D11*          m_pLiquidVrDevice;
    ALVRGpuAffinity*            m_pLiquidVRAffinity;
	HMODULE						m_hLiquidVRDLL;
};