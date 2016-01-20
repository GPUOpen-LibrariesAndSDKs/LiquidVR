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
#include "../inc/BarrelDistortionShader.h"

//-------------------------------------------------------------------------------------------------
BarrelDistortionShader::BarrelDistortionShader(float scaling) :
ImageShader2D(L"Shaders\\Source\\BarrelDistortion.hlsl", L"CSMain"),
m_pConstantBufferLR(NULL),
m_Width(0),
m_Height(0),
m_Scaling(1.4),
m_bAntialiasing(true)
{
    memset(&m_ParamsLR, 0, sizeof(m_ParamsLR));

    m_ParamsLR.k1Rgb = XMFLOAT3(1.09f, 1.23f, 1.4f); // Good set of parameters for Oculus lenses
    m_ParamsLR.zoomSrcToDst = 1.0f / m_Scaling;
    m_ParamsLR.k2Rgb = XMFLOAT3(1.24f, 1.245f, 1.25f);
    m_ParamsLR.enableAntialias = m_bAntialiasing ? 1 : 0;
    m_ParamsLR.backgrndColor = XMFLOAT4(0.0f, 0.0f, 0.9f, 0.5f);
}
//-------------------------------------------------------------------------------------------------
BarrelDistortionShader::~BarrelDistortionShader()
{
}
//-------------------------------------------------------------------------------------------------
void    BarrelDistortionShader::ResetSize()
{
    m_Width = 0;
    m_Height = 0;
}
//-------------------------------------------------------------------------------------------------
HRESULT BarrelDistortionShader::QueryEyes(UINT srcWidth, UINT srcHeight, UINT dstWidth, UINT dstHeight, ALVRSurface *pInputSurfaceL, ALVRSurface *pInputSurfaceR, ALVRSurface *pOutputSurfaceL, ALVRSurface *pOutputSurfaceR, bool bWait)
{
    bool srcVert = srcHeight > srcWidth;
    bool dstVert = dstHeight > dstWidth;
    UINT srcEyeWidth = srcWidth;
    UINT srcEyeHeight = srcHeight;
    UINT dstEyeWidth = dstWidth;
    UINT dstEyeHeight = dstHeight;
    if (srcVert)
    {
        srcEyeHeight /= 2;
    }
    else
    {
        srcEyeWidth /= 2;
    }
    if (dstVert)
    {
        dstEyeHeight /= 2;
    }
    else
    {
        dstEyeWidth /= 2;
    }
    if (m_Width != dstWidth || m_Height != dstHeight)
    {
        m_Width = dstWidth;
        m_Height = dstHeight;

        if (srcVert)
        {
            m_ParamsLR.inputPosL = XMINT2((int)0, (int)0);
            m_ParamsLR.inputPosR = XMINT2((int)0, (int)srcEyeHeight);
        }
        else
        {
            m_ParamsLR.inputPosL = XMINT2((int)0, (int)0);
            m_ParamsLR.inputPosR = XMINT2((int)srcEyeWidth, (int)0);
        }

        if (dstVert)
        {
            m_ParamsLR.displayPosL = XMINT2((int)0, (int)0);
            m_ParamsLR.displayPosR = XMINT2((int)0, (int)dstEyeHeight);

        }
        else
        {
            m_ParamsLR.displayPosL = XMINT2((int)0, (int)0);
            m_ParamsLR.displayPosR = XMINT2((int)dstEyeWidth, (int)0);
        }

        m_ParamsLR.origOutL = XMFLOAT2(.5f - .5f / dstEyeWidth, .5f - .5f / dstEyeHeight);
        m_ParamsLR.origOutR = XMFLOAT2(.5f - .5f / dstEyeWidth, .5f - .5f / dstEyeHeight);
        m_ParamsLR.displaySizeL = XMINT2((int)dstEyeWidth, (int)dstEyeHeight);
        m_ParamsLR.displaySizeR = XMINT2((int)dstEyeWidth, (int)dstEyeHeight);
        m_ParamsLR.inputSizeL = XMINT2((int)srcEyeWidth, (int)srcEyeHeight);
        m_ParamsLR.inputSizeR = XMINT2((int)srcEyeWidth, (int)srcEyeHeight);

        void *pBuffPtr = NULL;
        m_pConstantBufferLR->Map(&pBuffPtr);
        memcpy(pBuffPtr, &m_ParamsLR, sizeof(m_ParamsLR));
        m_pConstantBufferLR->Unmap();

    }

    ALVRBuffer*	pConstantBuffer = m_pConstantBufferLR;
    m_pShader->BindConstantBuffer(0, pConstantBuffer);

//    return QueryLR(dstEyeWidth * 2, dstEyeHeight, pInputSurfaceL, pInputSurfaceR, pOutputSurfaceL, pOutputSurfaceR, bWait);  // Grid Size is always for [L|R] (horizontal) orientation of left/righr eye

    m_pShader->BindInput(0, pInputSurfaceL);
    m_pShader->BindInput(1, pInputSurfaceR);
    m_pShader->BindOutput(0, pOutputSurfaceL);
    m_pShader->BindOutput(1, pOutputSurfaceR);

    ALVRPoint3D offset ={0, 0, 0};
    ALVRSize3D size ={(dstEyeWidth * 2 + THREAD_GROUP_SIZE - 1) / THREAD_GROUP_SIZE, (dstEyeHeight + THREAD_GROUP_SIZE - 1) / THREAD_GROUP_SIZE, 0};
    m_pComputeContext->QueueTask(m_pShader, &offset, &size);

    if(bWait)
    {
        m_pComputeContext->Flush(m_pFence);
        m_pFence->Wait(2000);
    }
    else
    {
        //        m_pComputeContext->Flush(NULL);
    }
    return S_OK;
}

//-------------------------------------------------------------------------------------------------
HRESULT BarrelDistortionShader::Start()
{
    ALVR_RESULT res = ALVR_OK;

    HRESULT hr = ImageShader2D::Start();
    if (m_bShaderReady && SUCCEEDED(hr))
    {
        ALVRBufferDesc descBuffLR = { 0 };
        descBuffLR.size = sizeof(ParametersLR);
        descBuffLR.apiSupport = ALVR_RESOURCE_API_ASYNC_COMPUTE;
        descBuffLR.bufferFlags = ALVR_BUFFER_CONSTANT;
        descBuffLR.cpuAccessFlags = ALVR_CPU_ACCESS_WRITE;
        descBuffLR.structureStride = 0;
        res = m_pComputeContext->CreateBuffer(&descBuffLR, &m_pConstantBufferLR);


        m_Width = 0;
        m_Height = 0;
    }
    return hr;
}
//-------------------------------------------------------------------------------------------------
HRESULT BarrelDistortionShader::Stop()
{
    if (m_pConstantBufferLR != NULL)
    {
        m_pConstantBufferLR->Release();
        m_pConstantBufferLR = NULL;
    }
    if (m_pShader != NULL)
    {
        m_pShader->Release();
        m_pShader = NULL;
    }

    return ImageShader2D::Stop();
}
//-------------------------------------------------------------------------------------------------
bool BarrelDistortionShader::GetAntialiasing()
{
    return m_bAntialiasing;
}
void BarrelDistortionShader::SetAntialiasing(bool bAntialiasing)
{
    m_bAntialiasing = bAntialiasing;
    if (m_pConstantBufferLR != NULL)
    {
        m_ParamsLR.enableAntialias = m_bAntialiasing ? 1 : 0;

        void *pBuffPtr = NULL;
        m_pConstantBufferLR->Map(&pBuffPtr);
        memcpy(pBuffPtr, &m_ParamsLR, sizeof(m_ParamsLR));
        m_pConstantBufferLR->Unmap();
    }
}
//-------------------------------------------------------------------------------------------------
void BarrelDistortionShader::GetCoeffs(float *red, float *green, float *blue)
{
    *red = m_ParamsLR.k1Rgb.x;
    *green = m_ParamsLR.k1Rgb.y;
    *blue = m_ParamsLR.k1Rgb.z;
}
//-------------------------------------------------------------------------------------------------
void BarrelDistortionShader::SetCoeffs(float red, float green, float blue)
{
    m_ParamsLR.k1Rgb.x = red;
    m_ParamsLR.k1Rgb.y = green;
    m_ParamsLR.k1Rgb.z = blue;

    void *pBuffPtr = NULL;
    m_pConstantBufferLR->Map(&pBuffPtr);
    memcpy(pBuffPtr, &m_ParamsLR, sizeof(m_ParamsLR));
    m_pConstantBufferLR->Unmap();

}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
