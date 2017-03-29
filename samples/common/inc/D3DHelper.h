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

#include "../../../inc/LiquidVR.h"
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <float.h>
#include <atlbase.h>


class D3DHelper
{
public:
    struct ColorVertex
    {
        DirectX::XMFLOAT3 Pos;
        DirectX::XMFLOAT4 Col;
    };

    struct TextureVertex
    {
        DirectX::XMFLOAT3 Pos;
        DirectX::XMFLOAT2 Tex;
    };

    struct ConstantBuffer
    {
        DirectX::XMMATRIX m_WorldViewProjection;
    };

    D3DHelper();
    virtual ~D3DHelper();

    virtual ALVR_RESULT CreateD3D11Device();
    virtual ALVR_RESULT CreateSwapChain(HWND hWindow, unsigned int iBackbufferCount);
    virtual ALVR_RESULT Create3DScene(unsigned int width, unsigned int height, bool bColorCube);

    virtual ALVR_RESULT Animate();

    virtual ALVR_RESULT SetupViewportForEye(bool bLeft);
    virtual ALVR_RESULT SetupMatrixForEye(bool bLeft);
    virtual ALVR_RESULT RenderFrame(int iDrawRepeat);
    virtual ALVR_RESULT Present();

    virtual ALVR_RESULT Terminate();

public:
    CComPtr<ID3D11Device>                m_pd3dDevice;
    CComPtr<ID3D11DeviceContext>         m_pd3dDeviceContext;
    CComPtr<IDXGISwapChain>              m_pSwapChain;
    ATL::CComPtr<ID3D11VertexShader>     m_pVertexShader;
    ATL::CComPtr<ID3D11PixelShader>      m_pPixelShader;
    ATL::CComPtr<ID3D11InputLayout>      m_pVertexLayout;
    ATL::CComPtr<ID3D11Buffer>           m_pVertexBuffer;
    ATL::CComPtr<ID3D11Buffer>           m_pIndexBuffer;
    ATL::CComPtr<ID3D11Buffer>           m_pConstantBuffer;
    ATL::CComPtr<ID3D11Texture2D>        m_pDepthStencil;
    ATL::CComPtr<ID3D11DepthStencilView> m_pDepthStencilView;
    CComPtr<ID3D11SamplerState>          m_pSampler;
    CComPtr<ID3D11RasterizerState>       m_pRasterizerState;

    float					             m_fAnimation;
    unsigned int                         m_Width;
    unsigned int                         m_Height;
    unsigned int                         m_iVertexCount;
protected:
    virtual const char *GetShaderText(bool bColorCube);
    virtual ALVR_RESULT CreateConstantBuffer();
    virtual ALVR_RESULT UpdateConstantBuffer(bool bLeft, DirectX::XMMATRIX &worldViewProjection);

    virtual ALVR_RESULT PrepareColorCube();
    virtual ALVR_RESULT PrepareTextureCube();

};

ALVR_RESULT ConvertFromatFromDXGIToALVR(DXGI_FORMAT dxgi, ALVR_FORMAT &alvr);
