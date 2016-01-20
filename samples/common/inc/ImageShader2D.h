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
#include "../../../inc/LiquidVRD2D.h"
#include "ApplicationContext.h"

#define THREAD_GROUP_SIZE 8 // the same as in shaders

class ImageShader2D
{
public:
    ImageShader2D(const WCHAR *pShaderName, const WCHAR *pMainName);
    virtual ~ImageShader2D();

    virtual HRESULT Init(ApplicationContext *pApplicationContext, ALVRComputeContext *pComputeContext);
    virtual HRESULT Terminate();
    virtual HRESULT Query(UINT width, UINT height, ALVRSurface *pInputSurface, ALVRSurface *pOutputSurface, bool bWait);
    virtual HRESULT Start();
    virtual HRESULT Stop();
    virtual bool    IsReady();

protected:
    static void    CALLBACK ShaderReadyCallback(_In_ UINT uiShaderID, _In_ void *pUserData);
    HRESULT ShaderReady(_In_ UINT uiShaderID);


    struct Surface
    {
        Surface() : m_pSurface(NULL), m_bDestroy(false), m_pRenderTargetView(NULL){}

        void Destroy()
        {
            if (m_pSurface != NULL)
            {
                if (m_bDestroy)
                {
                    m_pSurface->Release();
                }
                m_pSurface = NULL;
            }
            if (m_pRenderTargetView != NULL)
            {
                m_pRenderTargetView->Release();
                m_pRenderTargetView = NULL;
            }
        }

        ALVRSurface*            m_pSurface;
        bool                    m_bDestroy;
        ID3D11RenderTargetView* m_pRenderTargetView;
    };

    ApplicationContext*                 m_pApplicationContext;
    ALVRComputeContext*                 m_pComputeContext;
    ALVRComputeTask*                    m_pShader;
    ALVRFence*			                m_pFence;
    bool                                m_bShaderReady;
    UINT                                m_uiShaderID;
    bool                                m_bDelayedStart;
    WCHAR						        m_pFileNameSource[AMD::ShaderCache::m_uFILENAME_MAX_LENGTH];
    WCHAR						        m_pFileNameBin[AMD::ShaderCache::m_uFILENAME_MAX_LENGTH];
    WCHAR						        m_pNameMain[AMD::ShaderCache::m_uFILENAME_MAX_LENGTH];

};