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

#include "DXUT.h"
#include "DXUTmisc.h"
#include "DXUTCamera.h"
#include "SDKmisc.h"
#include "SDKmesh.h"

using namespace DirectX;

#include "ApplicationContext.h"

class Character
{
public:
	Character();
	virtual ~Character();
	virtual HRESULT Init(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, ApplicationContext *pContext);
	virtual HRESULT Terminate();
	virtual HRESULT Render(ID3D11DeviceContext* pd3dImmediateContext, float fbWidth, float fbHeight);
	HRESULT Time();
	virtual HRESULT SetPosition(float x, float y);


	// options 
	void SetShift(bool bShift);
	void SetAlpha(bool bAlpha){m_bAlpha = bAlpha;}
	void SetClear(bool bClear){ m_bClear = bClear;}
	void SetRotate(bool bRotate){ m_bRotate = bRotate; }

	bool GetShift(){ return m_bShift; }
	bool GetAlpha(){ return m_bAlpha; }
	bool GetClear(){ return m_bClear; }
	bool GetRotate(){ return m_bRotate; }

	void GetDrawLimits(int *iMinDraws, int *iMaxDraws){ *iMinDraws = m_iMinDraws; *iMaxDraws = m_iMaxDraws; }
	void SetTodoDraw(int iTodoDraws){ m_iTodoDraws = iTodoDraws; }
	int GetTodoDraw(){ return m_iTodoDraws; }

	void SetUseTimers(bool bUseTimers){ m_bUseTimers = bUseTimers; }

protected:
	void SetupEye(ID3D11DeviceContext* pd3dImmediateContext, GpuMask mask, bool bPortrait);
	void SetupPS(ID3D11DeviceContext* pd3dImmediateContext, GpuMask mask);
	void UpdatePositionBuffers(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext);
	void ClearPositionBuffers();
	void DrawObjectsForView(ID3D11DeviceContext* pd3dImmediateContext);

	virtual HRESULT AllocVShaderObjects(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext);
	virtual HRESULT UpdateVShaderObjects(ID3D11DeviceContext* pd3dImmediateContext, XMMATRIX &viewProj, XMMATRIX &mWorld, GpuMask mask);
	virtual WCHAR* GetVShaderName();
	virtual HRESULT SetupVS(ID3D11DeviceContext* pd3dImmediateContext, GpuMask mask);

	struct CB_VS_PER_OBJECT
	{
		XMFLOAT4X4	m_ViewProj;
		XMFLOAT4X4	m_World;
	};
	static UINT                        m_iCBVSPerObjectBind;

	ApplicationContext*			m_pContext;
	CDXUTSDKMesh				m_Mesh;
	ID3D11VertexShader*			m_pSimpleVS;
	ID3D11PixelShader*			m_pSimplePS;
	ID3D11InputLayout*			m_pVertexLayout;
	ID3D11Buffer*				m_pcbVSPerObject;
	ID3D11Buffer*				m_pcbPSPerObject;
	ID3D11Buffer*				m_pcbPSPerFrame;


	ID3D11Buffer**				m_pcbVSPosition;
	int							m_iPositionCount;
	int							m_iPositionAllocated;

	// options 
	bool						m_bShift;
	bool						m_bAlpha;
	bool						m_bClear;
	bool						m_bRotate;

	int							m_iMinDraws;
	int							m_iMaxDraws;
	int							m_iTodoDraws;
	bool						m_bUseTimers;

	// animation
	DWORD						m_dwTime;
	float						m_fTime;
	float						m_fPosX;
	float						m_fPosY;
};