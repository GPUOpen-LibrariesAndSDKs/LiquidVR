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

//--------------------------------------------------------------------------------------
// AMD LiquidVR AffinityGPUControl Extension
//--------------------------------------------------------------------------------------
#include "../../../inc/LiquidVR.h"

enum GpuMask
{
	GPUMASK_LEFT = 0x1,
	GPUMASK_RIGHT = 0x2,
	GPUMASK_BOTH = (GPUMASK_LEFT | GPUMASK_RIGHT)
};

enum TransferSyncMode
{
    TRANSFER_SYNC_AUTOMATIC = 0,
    TRANSFER_SYNC_MANUAL    = 1,
    TRANSFER_SYNC_NONE      = 2,
};

class MGPUAffinity
{
public:

	MGPUAffinity();
	virtual ~MGPUAffinity();

    virtual HRESULT     Init(ALVRGpuAffinity *pAffinity, ID3D11Device** ppd3dDevice, ID3D11DeviceContext** pd3dImmediateContext);
	virtual HRESULT     Terminate();

    virtual HRESULT     CreateSyncObjects(ALVRDeviceExD3D11* pLiquidVrDevice);

	virtual HRESULT     SetRenderGpuMask(GpuMask mask);
	virtual HRESULT     SetupViewport(ID3D11DeviceContext* pd3dImmediateContext, GpuMask mask, float fbWidth, float fbHeight);

	GpuMask			    GetCurrentMask();
	void			    SetUseAffinity(bool bUse);
	bool			    GetUseAffinity();
    TransferSyncMode	GetSyncType();
    void			    SetSyncType(TransferSyncMode eMode);

	void			    TransferRightEye(ID3D11DeviceContext* pd3dImmediateContext, float fbWidth, float fbHeight);
    void                MarkResourceAsInstanced(ID3D11Resource* pResource);

	bool			    IsEnabled(){ return m_pLiquidVrDeviceContext != NULL; }

protected:
    ALVRDeviceExD3D11*          m_pLiquidVrDevice;
	ALVRMultiGpuDeviceContext*	m_pLiquidVrDeviceContext;
	bool						m_bUseAffinity;
	GpuMask						m_eCurrentMask;
    TransferSyncMode            m_eTransferSyncMode;

    ALVRGpuSemaphore*           m_pRenderComplete0;
    ALVRGpuSemaphore*           m_pTransferComplete0;
    ALVRGpuSemaphore*           m_pRenderComplete1;
    ALVRGpuSemaphore*           m_pTransferComplete1;
    bool                        m_bFrameLoop;

};
