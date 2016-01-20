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
#include "../inc/ImageShader2D.h"

//-------------------------------------------------------------------------------------------------
ImageShader2D::ImageShader2D(const WCHAR *pShaderName, const WCHAR *pMainName) :
    m_pApplicationContext(NULL),
    m_pComputeContext(NULL),
    m_pShader(NULL),
    m_pFence(NULL),
    m_bShaderReady(false),
    m_uiShaderID(0),
    m_bDelayedStart(false)
{
    wcscpy_s(m_pFileNameSource, _countof(m_pFileNameSource), pShaderName);
    wcscpy_s(m_pNameMain, _countof(m_pNameMain), pMainName);
    
}
//-------------------------------------------------------------------------------------------------
ImageShader2D::~ImageShader2D()
{
}
//-------------------------------------------------------------------------------------------------
HRESULT ImageShader2D::Init(ApplicationContext *pApplicationContext, ALVRComputeContext *pComputeContext)
{
    m_pApplicationContext = pApplicationContext;
    m_pComputeContext = pComputeContext;

    if (!m_bShaderReady)
    {
        if (m_uiShaderID == 0)
        {
            m_pApplicationContext->m_ShaderCache.AddShader(NULL,
                AMD::ShaderCache::SHADER_TYPE_COMPUTE, L"cs_5_0", m_pNameMain,
                m_pFileNameSource, 0, NULL, NULL, NULL, 0,
                0, -1, -1, true, &m_uiShaderID, ShaderReadyCallback, this);

            m_pApplicationContext->m_ShaderCache.GetShaderBinaryFileName(m_uiShaderID, m_pFileNameBin, (UINT)_countof(m_pFileNameBin));

        }
    }
    return S_OK;
}
//-------------------------------------------------------------------------------------------------
HRESULT ImageShader2D::Terminate()
{
    Stop();
    m_pApplicationContext = NULL;
    m_pComputeContext = NULL;
    return S_OK;
}
//-------------------------------------------------------------------------------------------------
HRESULT ImageShader2D::Query(UINT width, UINT height, ALVRSurface *pInputSurface, ALVRSurface *pOutputSurface, bool bWait)
{
    m_pShader->BindInput(0, pInputSurface);
    m_pShader->BindOutput(0, pOutputSurface);

    ALVRPoint3D offset = { 0, 0, 0 };
    ALVRSize3D size = { (width + THREAD_GROUP_SIZE- 1) / THREAD_GROUP_SIZE , (height  + THREAD_GROUP_SIZE- 1)  / THREAD_GROUP_SIZE, 0 };
    m_pComputeContext->QueueTask(m_pShader, &offset, &size);

    if (bWait)
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
HRESULT ImageShader2D::Start()
{
    ALVR_RESULT res = ALVR_OK;

    if (m_bShaderReady)
    {
        // read shader in memory
        FILE *file = NULL;
        _wfopen_s(&file, m_pFileNameBin, L"rb");
        if (file == NULL)
        {
            return E_FAIL;
        }
        fseek(file, 0, SEEK_END);
        int fileSize = ftell(file);
        fseek(file, 0, SEEK_SET);
        unsigned char * pShader = new unsigned char[fileSize];
        fread(pShader, 1, fileSize, file);
        fclose(file);

        res = m_pComputeContext->CreateComputeTask(ALVR_SHADER_MODEL_D3D11, 0, pShader, fileSize, &m_pShader);

        delete[] pShader;

        if (m_pFence != NULL)
        {
            m_pFence->Release();
            m_pFence = NULL;
        }
        m_pApplicationContext->m_pLiquidVrDevice->CreateFence(&m_pFence);
    }
    else
    {
        m_bDelayedStart = true;
    }
    return S_OK;
}
//-------------------------------------------------------------------------------------------------
HRESULT ImageShader2D::Stop()
{
    if (m_pFence != NULL)
    {
        m_pFence->Release();
        m_pFence = NULL;
    }

    if (m_pShader != NULL)
    {
        m_pShader->Release();
        m_pShader = NULL;
    }
    return S_OK;
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void    CALLBACK ImageShader2D::ShaderReadyCallback(_In_ UINT uiShaderID, _In_ void *pUserData)
{
    ((ImageShader2D*)pUserData)->ShaderReady(uiShaderID);
}
//-------------------------------------------------------------------------------------------------
HRESULT ImageShader2D::ShaderReady(_In_ UINT uiShaderID)
{
    if (uiShaderID != m_uiShaderID)
    {
        return S_OK;
    }
    m_bShaderReady = true;
    if (m_bDelayedStart)
    {
        Start();
        m_bDelayedStart = false;
    }
    else if (m_pShader != NULL)
    {
        Stop();
        Start();
    }
    return S_OK;
}
//-------------------------------------------------------------------------------------------------
bool    ImageShader2D::IsReady()
{
    return m_pShader != NULL;
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
