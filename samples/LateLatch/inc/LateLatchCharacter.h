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
#include "..\..\common\inc\Character.h"

#include "../../../inc/LiquidVR.h"

class LateLatchCharacter : public Character
{
public:
	LateLatchCharacter();
	virtual ~LateLatchCharacter();

	virtual HRESULT Terminate();
	virtual HRESULT Render(ID3D11DeviceContext* pd3dImmediateContext, float fbWidth, float fbHeight);
	virtual HRESULT SetPosition(float x, float y);
	void	SetLateLatch(bool enable) { m_bLateLatch = enable; }
	bool	GetLateLatch(){ return m_bLateLatch; }
	void	SetLateLatchLeftEyeOnly(bool enable) { m_bLateLatchLeftEyeOnly = enable; }
	bool	GetLateLatchLeftEyeOnly(){ return m_bLateLatchLeftEyeOnly; }
	UINT	GetNumberOfLateLatchBuffers(){ return m_uiNumberOfLateLatchBuffers; }
	UINT	GetUpdatesPerFrame(){ return m_iUpdatesPerFrame; }
	

protected:
	virtual HRESULT AllocVShaderObjects(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext);
	virtual WCHAR* GetVShaderName();
	virtual HRESULT UpdateVShaderObjects(ID3D11DeviceContext* pd3dImmediateContext, XMMATRIX &worldViewProj, XMMATRIX &mWorld, GpuMask mask);
	virtual HRESULT SetupVS(ID3D11DeviceContext* pd3dImmediateContext, GpuMask mask);

    ALVRLateLatchConstantBufferD3D11 *m_pConstantBufferLeft;
    ALVRLateLatchConstantBufferD3D11 *m_pConstantBufferRight;

	static UINT m_iCBVSSIndexLatch;
	bool		m_bLateLatch;
	bool		m_bLateLatchLeftEyeOnly;
	static UINT m_uiNumberOfLateLatchBuffers;
	UINT		m_iUpdatesPerFrame;
};