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
#include "../inc/GridGeneratorShader.h"

//-------------------------------------------------------------------------------------------------
GridGeneratorShader::GridGeneratorShader() :
    ImageShader2D(L"Shaders\\Source\\Grid.hlsl", L"main"),
    m_pConstantBuffer(NULL),
    m_pSampler(NULL)
{
}
//-------------------------------------------------------------------------------------------------
GridGeneratorShader::~GridGeneratorShader()
{
}
//-------------------------------------------------------------------------------------------------
HRESULT GridGeneratorShader::Query(UINT width, UINT height, ALVRSurface *pInputSurface, ALVRSurface *pOutputSurface, bool bWait)
{
    if (m_Params.width != width || m_Params.height != height)
    {
        m_Params.width = width;
        m_Params.height = height;

        void *pBuffPtr = NULL;
        m_pConstantBuffer->Map(&pBuffPtr);
        memcpy(pBuffPtr, &m_Params, sizeof(m_Params));
        m_pConstantBuffer->Unmap();
    }
    return ImageShader2D::Query(width, height, pInputSurface, pOutputSurface, bWait);
}
//-------------------------------------------------------------------------------------------------
HRESULT GridGeneratorShader::Start()
{
    ALVR_RESULT res = ALVR_OK;

    HRESULT hr = ImageShader2D::Start();
    if (m_bShaderReady && SUCCEEDED(hr))
    { 

        
        ALVRBufferDesc descBuff = {0};
        descBuff.size = sizeof(Parameters);
        descBuff.apiSupport = ALVR_RESOURCE_API_ASYNC_COMPUTE;
        descBuff.bufferFlags = ALVR_BUFFER_CONSTANT;
        descBuff.cpuAccessFlags = ALVR_CPU_ACCESS_WRITE;
        descBuff.structureStride = 0;
        descBuff.format = ALVR_FORMAT_UNKNOWN;
        res = m_pComputeContext->CreateBuffer(&descBuff, &m_pConstantBuffer);
        /*
        D3D11_BUFFER_DESC bufDescDX11 = {};
        bufDescDX11.ByteWidth = sizeof(Parameters); // 512;
        bufDescDX11.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bufDescDX11.Usage = D3D11_USAGE_DEFAULT;
        bufDescDX11.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
        bufDescDX11.StructureByteStride = bufDescDX11.ByteWidth;// 16;
        bufDescDX11.CPUAccessFlags =  0;

        ID3D11Buffer *pBuffer;
        ::DXUTGetD3D11Device()->CreateBuffer(&bufDescDX11, NULL, &pBuffer);

        IDXGIResource *pResource;
        pBuffer->QueryInterface(__uuidof(IDXGIResource), (void**)&pResource);

        HANDLE hShared; 
        pResource->GetSharedHandle(&hShared);

        ALVROpenBufferDesc descOpen = {};
        descOpen.sharedHandle = hShared;
        descOpen.format = ALVR_FORMAT_UNKNOWN;
        descOpen.bufferFlags = ALVR_BUFFER_CONSTANT;
        descOpen.structureStride = 16;

        m_pComputeContext->OpenSharedBuffer(&descOpen, &m_pConstantBuffer);

        pResource->Release();
        */
        res = m_pShader->BindConstantBuffer(0, m_pConstantBuffer);

        m_Params.width = 0;
        m_Params.height = 0;
        m_Params.gridWidth = 30;
        m_Params.lineWidth = 1;
        XMVECTORF32 colorBg = Colors::Black;
        XMStoreFloat4(&m_Params.colorBg, colorBg);
        XMVECTORF32 colorFg = Colors::LightGray;
        XMStoreFloat4(&m_Params.colorFg, colorFg);

        ALVRSamplerDesc samplerDesc = {};
        samplerDesc.filterMode = ALVR_FILTER_POINT;
        samplerDesc.addressU = ALVR_ADDRESS_CLAMP;
        samplerDesc.addressV = ALVR_ADDRESS_CLAMP;
        samplerDesc.addressW = ALVR_ADDRESS_CLAMP;
        samplerDesc.maxAnisotropy = 1;

        m_pComputeContext->CreateSampler(&samplerDesc, &m_pSampler);

        m_pShader->BindSampler(0, m_pSampler);
    }
    return hr;
}
//-------------------------------------------------------------------------------------------------
HRESULT GridGeneratorShader::Stop()
{
    if (m_pConstantBuffer != NULL)
    {
        m_pConstantBuffer->Release();
        m_pConstantBuffer = NULL;
    }
    if (m_pSampler != NULL)
    {
        m_pSampler->Release();
        m_pSampler = NULL;
    }
    return ImageShader2D::Stop();
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
