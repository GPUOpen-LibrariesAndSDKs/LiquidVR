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
#include "../inc/D3DHelper.h"
#include "../inc/LvrLogger.h"

//#define UPDATE_RESOURCE

using namespace DirectX;

//-------------------------------------------------------------------------------------------------
D3DHelper::D3DHelper() :
m_fAnimation(0),
m_Width(0),
m_Height(0),
m_iVertexCount(0)
{

}
//-------------------------------------------------------------------------------------------------
D3DHelper::~D3DHelper()
{

}
//-------------------------------------------------------------------------------------------------
ALVR_RESULT D3DHelper::CreateD3D11Device()
{
    HRESULT hr = S_OK;
    ALVR_RESULT res = ALVR_OK;

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0
    };
    D3D_FEATURE_LEVEL featureLevel;

    D3D_DRIVER_TYPE eDriverType = D3D_DRIVER_TYPE_HARDWARE;

    hr = D3D11CreateDevice(NULL, eDriverType, NULL, createDeviceFlags, featureLevels, _countof(featureLevels),
        D3D11_SDK_VERSION, &m_pd3dDevice, &featureLevel, &m_pd3dDeviceContext);

#ifdef _DEBUG
    if(FAILED(hr))
    {
        LOG_INFO(L"InitDX11() failed to create HW DX11.1 debug device ");
        createDeviceFlags &= (~D3D11_CREATE_DEVICE_DEBUG);
        hr = D3D11CreateDevice(NULL, eDriverType, NULL, createDeviceFlags, featureLevels, _countof(featureLevels),
            D3D11_SDK_VERSION, &m_pd3dDevice, &featureLevel, &m_pd3dDeviceContext);
    }
#endif

    if(FAILED(hr))
    {
        LOG_INFO(L"InitDX11() failed to create HW DX11.1 release device ");
#ifdef _DEBUG
        createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
        hr = D3D11CreateDevice(NULL, eDriverType, NULL, createDeviceFlags, featureLevels + 1, _countof(featureLevels) - 1,
            D3D11_SDK_VERSION, &m_pd3dDevice, &featureLevel, &m_pd3dDeviceContext);
    }
#ifdef _DEBUG
    if(FAILED(hr))
    {
        LOG_INFO(L"InitDX11() failed to create HW DX11 debug device ");
        createDeviceFlags &= (~D3D11_CREATE_DEVICE_DEBUG);
        hr = D3D11CreateDevice(NULL, eDriverType, NULL, createDeviceFlags, featureLevels + 1, _countof(featureLevels) - 1,
            D3D11_SDK_VERSION, &m_pd3dDevice, &featureLevel, &m_pd3dDeviceContext);
    }
#endif
    CHECK_HRESULT_ERROR_RETURN(hr, L"InitDX11() failed to create HW DX11 release device ");

    return ALVR_OK;
}
//-------------------------------------------------------------------------------------------------
ALVR_RESULT D3DHelper::CreateSwapChain(HWND hWindow, unsigned int iBackbufferCount)
{
    HRESULT hr = S_OK;
    CComQIPtr<IDXGIDevice> spDXGIDevice=m_pd3dDevice;

    CComPtr<IDXGIAdapter> pDXGIAdapter;
    hr = spDXGIDevice->GetParent(__uuidof(IDXGIAdapter), (void **)&pDXGIAdapter);
    CHECK_HRESULT_ERROR_RETURN(hr, L"GetParent(__uuidof(IDXGIAdapter)) failed");

    CComPtr<IDXGIFactory> pIDXGIFactory;
    hr = pDXGIAdapter->GetParent(__uuidof(IDXGIFactory), (void **)&pIDXGIFactory);
    CHECK_HRESULT_ERROR_RETURN(hr, L"GetParent(__uuidof(IDXGIFactory)) failed");

    // setup params
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = iBackbufferCount;
    sd.BufferDesc.Width = 0;    // will get from window
    sd.BufferDesc.Height = 0;   // will get from window
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWindow;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    sd.Flags=DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    hr = pIDXGIFactory->CreateSwapChain(m_pd3dDevice, &sd, &m_pSwapChain);
    CHECK_HRESULT_ERROR_RETURN(hr, L"CreateSwapChain() failed");

    hr = pIDXGIFactory->MakeWindowAssociation(hWindow, DXGI_MWA_NO_ALT_ENTER);
    CHECK_HRESULT_ERROR_RETURN(hr, L"Failed to set DXGI_MWA_NO_ALT_ENTER");

    return ALVR_OK;
}
//-------------------------------------------------------------------------------------------------
ALVR_RESULT D3DHelper::Create3DScene(unsigned int width, unsigned int height, bool bColorCube)
{
    HRESULT hr = S_OK;
    ALVR_RESULT res = ALVR_OK;

    m_Width = width;
    m_Height = height;

    //---------------------------------------------------------------------------------------------
    // Prepare cube
    //---------------------------------------------------------------------------------------------

    if(bColorCube)
    { 
        res = PrepareColorCube();
        CHECK_ALVR_ERROR_RETURN(res, L"PrepareColorCube() failed");
    }
    else
    {
        res = PrepareTextureCube();
        CHECK_ALVR_ERROR_RETURN(res, L"PrepareTextureCube() failed");
    }
    m_pd3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_pd3dDeviceContext->VSSetShader(m_pVertexShader, NULL, 0);
    m_pd3dDeviceContext->PSSetShader(m_pPixelShader, NULL, 0);

    //---------------------------------------------------------------------------------------------
    // Create the constant buffer
    //---------------------------------------------------------------------------------------------
    res = CreateConstantBuffer();

    //---------------------------------------------------------------------------------------------
    // Create depth
    //---------------------------------------------------------------------------------------------

    D3D11_TEXTURE2D_DESC descDepth;
    descDepth.Width = width;
    descDepth.Height = height;
    descDepth.MipLevels = 1;
    descDepth.ArraySize = 1;
    descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    descDepth.SampleDesc.Count = 1;
    descDepth.SampleDesc.Quality = 0;
    descDepth.Usage = D3D11_USAGE_DEFAULT;
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    descDepth.CPUAccessFlags = 0;
    descDepth.MiscFlags = 0;
    hr = m_pd3dDevice->CreateTexture2D(&descDepth, nullptr, &m_pDepthStencil);
    CHECK_HRESULT_ERROR_RETURN(hr, L"CreateTexture2D() - depth failed");

    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
    descDSV.Format = descDepth.Format;
    descDSV.Flags = 0;
    descDSV.ViewDimension = descDepth.SampleDesc.Count > 1 ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D;
    descDSV.Texture2D.MipSlice = 0;

    hr = m_pd3dDevice->CreateDepthStencilView(m_pDepthStencil, &descDSV, &m_pDepthStencilView);
    CHECK_HRESULT_ERROR_RETURN(hr, L"CreateDepthStencilView() - depth failed");

    return res;
}
//-------------------------------------------------------------------------------------------------
ALVR_RESULT D3DHelper::PrepareColorCube()
{
    HRESULT hr = S_OK;
    ALVR_RESULT res = ALVR_OK;

    ATL::CComPtr<ID3DBlob> pVSBlob;
    ATL::CComPtr<ID3DBlob> pVSErrorBlob;

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;

    const char* pShaderText = GetShaderText(true);
    //---------------------------------------------------------------------------------------------
    // Create the vertex shader
    //---------------------------------------------------------------------------------------------
    hr = D3DCompile((LPCSTR)pShaderText, strlen(pShaderText), NULL, NULL, NULL, "VS", "vs_4_0", dwShaderFlags, 0, &pVSBlob, &pVSErrorBlob);
    CHECK_HRESULT_ERROR_RETURN(hr, L"D3DCompile(VS) failed" << reinterpret_cast<char*>(pVSErrorBlob->GetBufferPointer()));

    hr = m_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &m_pVertexShader);
    CHECK_HRESULT_ERROR_RETURN(hr, L"CreateVertexShader() failed");

    //---------------------------------------------------------------------------------------------
    // Create the input layout
    //---------------------------------------------------------------------------------------------
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    hr = m_pd3dDevice->CreateInputLayout(layout, _countof(layout), pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &m_pVertexLayout);
    CHECK_HRESULT_ERROR_RETURN(hr, L"CreateInputLayout() failed");

    m_pd3dDeviceContext->IASetInputLayout(m_pVertexLayout);

    //---------------------------------------------------------------------------------------------
    // Create pixel shader
    //---------------------------------------------------------------------------------------------
    ATL::CComPtr<ID3DBlob> pPSBlob;
    ATL::CComPtr<ID3DBlob> pPSErrorBlob;

    hr = D3DCompile((LPCSTR)pShaderText, strlen(pShaderText), NULL, NULL, NULL, "PS", "ps_4_0", dwShaderFlags, 0, &pPSBlob, &pPSErrorBlob);
    CHECK_HRESULT_ERROR_RETURN(hr, L"D3DCompile(PS) failed" << reinterpret_cast<char*>(pPSErrorBlob->GetBufferPointer()));

    hr = m_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &m_pPixelShader);
    CHECK_HRESULT_ERROR_RETURN(hr, L"CreatePixelShader(PS) failed");

    //---------------------------------------------------------------------------------------------
    // Create and set vertex buffer
    //---------------------------------------------------------------------------------------------
    ColorVertex vertices[] =
    {
        {XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f)},
        {XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f)},
        {XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f)},
        {XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f)},
        {XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f)},
        {XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f)},
        {XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f)},
        {XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f)},
    };
    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(ColorVertex) * _countof(vertices);
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA InitData = {};
    InitData.pSysMem = vertices;

    hr = m_pd3dDevice->CreateBuffer(&bd, &InitData, &m_pVertexBuffer);
    CHECK_HRESULT_ERROR_RETURN(hr, L"CreateBuffer() - vertex failed");

    UINT stride = sizeof(ColorVertex);
    UINT offset = 0;
    m_pd3dDeviceContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer.p, &stride, &offset);

    //---------------------------------------------------------------------------------------------
    // Create and set index buffer
    //---------------------------------------------------------------------------------------------
    WORD indices[] = {  // 36 vertices needed for 12 triangles in a triangle list
    3, 1, 0, 2, 1, 3,
    0, 5, 4, 1, 5, 0,
    3, 4, 7, 0, 4, 3,
    1, 6, 5, 2, 6, 1,
    2, 7, 6, 3, 7, 2,
    6, 4, 5, 7, 4, 6,
    };
    m_iVertexCount = _countof(indices);

    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(WORD) * m_iVertexCount;       
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.CPUAccessFlags = 0;

    InitData.pSysMem = indices;
    hr = m_pd3dDevice->CreateBuffer(&bd, &InitData, &m_pIndexBuffer);
    CHECK_HRESULT_ERROR_RETURN(hr, L"CreateBuffer() - index failed");

    m_pd3dDeviceContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

    return ALVR_OK;
}
//-------------------------------------------------------------------------------------------------
ALVR_RESULT D3DHelper::PrepareTextureCube()
{
    HRESULT hr = S_OK;
    ALVR_RESULT res = ALVR_OK;

    ATL::CComPtr<ID3DBlob> pVSBlob;
    ATL::CComPtr<ID3DBlob> pVSErrorBlob;

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;

    const char* pShaderText = GetShaderText(false);
    //---------------------------------------------------------------------------------------------
    // Create the vertex shader
    //---------------------------------------------------------------------------------------------
    hr = D3DCompile((LPCSTR)pShaderText, strlen(pShaderText), NULL, NULL, NULL, "VS", "vs_4_0", dwShaderFlags, 0, &pVSBlob, &pVSErrorBlob);
    CHECK_HRESULT_ERROR_RETURN(hr, L"D3DCompile(VS) failed" << reinterpret_cast<char*>(pVSErrorBlob->GetBufferPointer()));

    hr = m_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &m_pVertexShader);
    CHECK_HRESULT_ERROR_RETURN(hr, L"CreateVertexShader() failed");

    //---------------------------------------------------------------------------------------------
    // Create the input layout
    //---------------------------------------------------------------------------------------------
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    hr = m_pd3dDevice->CreateInputLayout(layout, _countof(layout), pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &m_pVertexLayout);
    CHECK_HRESULT_ERROR_RETURN(hr, L"CreateInputLayout() failed");

    m_pd3dDeviceContext->IASetInputLayout(m_pVertexLayout);

    //---------------------------------------------------------------------------------------------
    // Create pixel shader
    //---------------------------------------------------------------------------------------------
    ATL::CComPtr<ID3DBlob> pPSBlob;
    ATL::CComPtr<ID3DBlob> pPSErrorBlob;

    hr = D3DCompile((LPCSTR)pShaderText, strlen(pShaderText), NULL, NULL, NULL, "PS", "ps_4_0", dwShaderFlags, 0, &pPSBlob, &pPSErrorBlob);
    CHECK_HRESULT_ERROR_RETURN(hr, L"D3DCompile(PS) failed" << reinterpret_cast<char*>(pPSErrorBlob->GetBufferPointer()));

    hr = m_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &m_pPixelShader);
    CHECK_HRESULT_ERROR_RETURN(hr, L"CreatePixelShader(PS) failed");

    //---------------------------------------------------------------------------------------------
    // Create and set vertex buffer
    //---------------------------------------------------------------------------------------------
    TextureVertex vertices[] =
    {
        {XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT2(0.0f, 0.0f)},
        {XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT2(1.0f, 0.0f)},
        {XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f)},
        {XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f)},

        {XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT2(0.0f, 0.0f)},
        {XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT2(1.0f, 0.0f)},
        {XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f)},
        {XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f)},

        {XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT2(0.0f, 0.0f)},
        {XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT2(1.0f, 0.0f)},
        {XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT2(1.0f, 1.0f)},
        {XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f)},

        {XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT2(0.0f, 0.0f)},
        {XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT2(1.0f, 0.0f)},
        {XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT2(1.0f, 1.0f)},
        {XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f)},

        {XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT2(0.0f, 0.0f)},
        {XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT2(1.0f, 0.0f)},
        {XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT2(1.0f, 1.0f)},
        {XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT2(0.0f, 1.0f)},

        {XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT2(0.0f, 0.0f)},
        {XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT2(1.0f, 0.0f)},
        {XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f)},
        {XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f)},

    };
    D3D11_BUFFER_DESC bd ={};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(vertices);
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA InitData ={};
    InitData.pSysMem = vertices;

    hr = m_pd3dDevice->CreateBuffer(&bd, &InitData, &m_pVertexBuffer);
    CHECK_HRESULT_ERROR_RETURN(hr, L"CreateBuffer() - vertex failed");

    UINT stride = sizeof(TextureVertex);
    UINT offset = 0;
    m_pd3dDeviceContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer.p, &stride, &offset);

    //---------------------------------------------------------------------------------------------
    // Create and set index buffer
    //---------------------------------------------------------------------------------------------
    WORD indices[] ={
        3, 1, 0,
        2, 1, 3,
        6, 4, 5,
        7, 4, 6,
        11, 9, 8,
        10, 9, 11,
        14, 12, 13,
        15, 12, 14,
        19, 17, 16,
        18, 17, 19,
        22, 20, 21,
        23, 20, 22
    };

    m_iVertexCount = _countof(indices);

    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(WORD) * m_iVertexCount;        // 36 vertices needed for 12 triangles in a triangle list
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.CPUAccessFlags = 0;

    InitData.pSysMem = indices;
    hr = m_pd3dDevice->CreateBuffer(&bd, &InitData, &m_pIndexBuffer);
    CHECK_HRESULT_ERROR_RETURN(hr, L"CreateBuffer() - index failed");

    m_pd3dDeviceContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

    //---------------------------------------------------------------------------------------------
    // create and set sampler
    //---------------------------------------------------------------------------------------------

    D3D11_SAMPLER_DESC sampDesc;
    ZeroMemory(&sampDesc, sizeof(sampDesc));
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    hr = m_pd3dDevice->CreateSamplerState(&sampDesc, &m_pSampler);
    CHECK_HRESULT_ERROR_RETURN(hr, L"CreateSamplerState() failed");

    m_pd3dDeviceContext->PSSetSamplers(0, 1, &m_pSampler.p);

    //---------------------------------------------------------------------------------------------
    // Create the rasterizer state which will determine how and what polygons will be drawn.
    //---------------------------------------------------------------------------------------------

    D3D11_RASTERIZER_DESC rasterDesc;
    memset(&rasterDesc, 0, sizeof(rasterDesc));
    rasterDesc.AntialiasedLineEnable = false;
    rasterDesc.CullMode = D3D11_CULL_BACK;
    rasterDesc.DepthBias = 0;
    rasterDesc.DepthBiasClamp = 0.0f;
    rasterDesc.DepthClipEnable = true;
    rasterDesc.FillMode = D3D11_FILL_SOLID;
    rasterDesc.FrontCounterClockwise = false;
    rasterDesc.MultisampleEnable = false;
    rasterDesc.ScissorEnable = false;
    rasterDesc.SlopeScaledDepthBias = 0.0f;

    // Create the rasterizer state from the description we just filled out.
    hr = m_pd3dDevice->CreateRasterizerState(&rasterDesc, &m_pRasterizerState);
    CHECK_HRESULT_ERROR_RETURN(hr, L"CreateRasterizerState() failed");

    m_pd3dDeviceContext->RSSetState(m_pRasterizerState);
    return ALVR_OK;
}
//-------------------------------------------------------------------------------------------------
ALVR_RESULT D3DHelper::CreateConstantBuffer()
{
    HRESULT hr = S_OK;
    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd));

    bd.ByteWidth = sizeof(ConstantBuffer);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
#ifdef UPDATE_RESOURCE
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.CPUAccessFlags = 0;
#else
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
#endif    
    hr = m_pd3dDevice->CreateBuffer(&bd, NULL, &m_pConstantBuffer);
    CHECK_HRESULT_ERROR_RETURN(hr, L"CreateBuffer() - constant failed");
    m_pd3dDeviceContext->VSSetConstantBuffers(0, 1, &m_pConstantBuffer.p);

    return ALVR_OK;
}
//-------------------------------------------------------------------------------------------------
ALVR_RESULT D3DHelper::SetupViewportForEye(bool bLeft)
{
    D3D11_VIEWPORT viewport;

    viewport.MinDepth = 0;
    viewport.MaxDepth = 1;

    if(m_Height > m_Width)
    {
        if(bLeft)
        {
            viewport.TopLeftX = 0;
            viewport.TopLeftY = 0;
            viewport.Width = (float)m_Width;
            viewport.Height = (float)m_Height / 2.0f;
        }
        else
        {
            viewport.TopLeftX = 0;
            viewport.TopLeftY = (float)m_Height / 2.0f;
            viewport.Width = (float)m_Width;
            viewport.Height = (float)m_Height / 2.0f;
        }
    }
    else
    {
        if(bLeft)
        {
            viewport.TopLeftX = 0;
            viewport.TopLeftY = 0;
            viewport.Width = (float)m_Width / 2.0f;
            viewport.Height = (float)m_Height;
        }
        else
        {
            viewport.TopLeftX = (float)m_Width / 2.0f;
            viewport.TopLeftY = 0;
            viewport.Width = (float)m_Width / 2.0f;
            viewport.Height = (float)m_Height;
        }
    }
    m_pd3dDeviceContext->RSSetViewports(1, &viewport);
    return ALVR_OK;
}
//-------------------------------------------------------------------------------------------------
ALVR_RESULT D3DHelper::Animate()
{
    //---------------------------------------------------------------------------------------------
    // Animate the cube
    //---------------------------------------------------------------------------------------------
    m_fAnimation += XM_2PI / 240;
    
    return ALVR_OK;
}
//-------------------------------------------------------------------------------------------------
ALVR_RESULT D3DHelper::SetupMatrixForEye(bool bLeft)
{
    ALVR_RESULT res = ALVR_OK;
    HRESULT hr = S_OK;
    XMMATRIX world = XMMatrixRotationRollPitchYaw(m_fAnimation, -m_fAnimation, 0);
    //---------------------------------------------------------------------------------------------
    // Setup matrixes
    //---------------------------------------------------------------------------------------------
    bool bPortrait = m_Width < m_Height;
    float width = bPortrait ? (float)m_Width : (float)m_Width / 2.f;
    float height = bPortrait ? (float)m_Height / 2.f : (float)m_Height;

    float eyeOffset = 0.5f;
    XMVECTOR Eye = bLeft ? XMVectorSet(-eyeOffset, 1.0f, -5.0f, 0.0f) : XMVectorSet(eyeOffset, 1.0f, -5.0f, 0.0f);
    XMVECTOR At = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMMATRIX view = XMMatrixLookAtLH(Eye, At, Up);
    XMMATRIX projection = XMMatrixPerspectiveFovLH(XM_PIDIV2, width / height, 0.01f, 100.0f);

    XMMATRIX worldViewProjection = world * view * projection;
    if(bPortrait)
    {
        worldViewProjection = worldViewProjection *XMMatrixRotationZ(XM_PIDIV2);
    }
    XMMATRIX worldViewProjectionTransponded = XMMatrixTranspose(worldViewProjection);;
    return UpdateConstantBuffer(bLeft, worldViewProjectionTransponded);
}
//-------------------------------------------------------------------------------------------------
ALVR_RESULT D3DHelper::UpdateConstantBuffer(bool bLeft, DirectX::XMMATRIX &worldViewProjection)
{
    //---------------------------------------------------------------------------------------------
    // Update variables
    //---------------------------------------------------------------------------------------------
    ConstantBuffer cb;
    cb.m_WorldViewProjection = worldViewProjection;
#ifdef UPDATE_RESOURCE
    m_pd3dDeviceContext->UpdateSubresource(m_pConstantBuffer, 0, NULL, &cb, 0, 0);
#else

    D3D11_MAPPED_SUBRESOURCE MappedResource;
    HRESULT hr = m_pd3dDeviceContext->Map(m_pConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
    CHECK_HRESULT_ERROR_RETURN(hr, L"Map() failed");
    ConstantBuffer *pCb = reinterpret_cast<ConstantBuffer*>(MappedResource.pData);
    memcpy(pCb, &cb, sizeof(cb));
    m_pd3dDeviceContext->Unmap(m_pConstantBuffer, 0);
#endif    
    return ALVR_OK;
}
//-------------------------------------------------------------------------------------------------
ALVR_RESULT D3DHelper::RenderFrame(int iDrawRepeat)
{
    ALVR_RESULT res = ALVR_OK;
    //---------------------------------------------------------------------------------------------
    // Render cube
    //---------------------------------------------------------------------------------------------
    for(int i = 0; i < iDrawRepeat; i++)
    {
        m_pd3dDeviceContext->DrawIndexed(m_iVertexCount, 0, 0);
    }
    return ALVR_OK;
}
//-------------------------------------------------------------------------------------------------
ALVR_RESULT D3DHelper::Present()
{
    CHECK_RETURN(m_pSwapChain != NULL, ALVR_NOT_INITIALIZED, L"m_pSwapChain == NULL");
    HRESULT hr = S_OK;
    hr = m_pSwapChain->Present(0, 0);
    CHECK_HRESULT_ERROR_RETURN(hr, L"Present() failed");

    return ALVR_OK;
}
//-------------------------------------------------------------------------------------------------
ALVR_RESULT D3DHelper::Terminate()
{
    m_pRasterizerState.Release();
    m_pSampler.Release();
    m_pDepthStencil.Release();
    m_pDepthStencilView.Release();

    m_pVertexShader.Release();
    m_pPixelShader.Release();
    m_pVertexLayout.Release();
    m_pVertexBuffer.Release();
    m_pIndexBuffer.Release();
    m_pConstantBuffer.Release();

    m_pSwapChain.Release();
    m_pd3dDeviceContext.Release();
    m_pd3dDevice.Release();

    return ALVR_OK;
}
//-------------------------------------------------------------------------------------------------
const char *D3DHelper::GetShaderText(bool bColorCube)
{
static const char s_pColorShaderText[] =
"//--------------------------------------------------------------------------------------\n"
"// Constant Buffer Variables                                                            \n"
"//--------------------------------------------------------------------------------------\n"
"cbuffer ConstantBuffer : register( b0 )                                                 \n"
"{                                                                                       \n"
"    matrix WorldViewProjection;                                                         \n"
"}                                                                                       \n"
"                                                                                        \n"
"//--------------------------------------------------------------------------------------\n"
"struct VS_OUTPUT                                                                        \n"
"{                                                                                       \n"
"    float4 Pos : SV_POSITION;                                                           \n"
"    float4 Color : COLOR0;                                                              \n"
"};                                                                                      \n"
"                                                                                        \n"
"//--------------------------------------------------------------------------------------\n"
"// Vertex Shader                                                                        \n"
"//--------------------------------------------------------------------------------------\n"
"VS_OUTPUT VS( float4 Pos : POSITION, float4 Color : COLOR )                             \n"
"{                                                                                       \n"
"    VS_OUTPUT output = (VS_OUTPUT)0;                                                    \n"
"    output.Pos = mul(Pos, WorldViewProjection);                                         \n"
"    output.Color = Color;                                                               \n"
"    return output;                                                                      \n"
"}                                                                                       \n"
"                                                                                        \n"
"                                                                                        \n"
"//--------------------------------------------------------------------------------------\n"
"// Pixel Shader                                                                         \n"
"//--------------------------------------------------------------------------------------\n"
"float4 PS( VS_OUTPUT input ) : SV_Target                                                \n"
"{                                                                                       \n"
"    return input.Color;                                                                 \n"
"}                                                                                       \n"
;
static const char s_pTextureShaderText[] =
"//--------------------------------------------------------------------------------------\n"
"// Constant Buffer Variables                                                            \n"
"//--------------------------------------------------------------------------------------\n"
"cbuffer ConstantBuffer : register( b0 )                                                 \n"
"{                                                                                       \n"
"    matrix WorldViewProjection;                                                         \n"
"}                                                                                       \n"
"//--------------------------------------------------------------------------------------\n"
"Texture2D txDiffuse : register( t0 );                                                   \n"
"SamplerState samplerState : register( s0 );                                             \n"
"//--------------------------------------------------------------------------------------\n"
"struct VS_INPUT                                                                         \n"
"{                                                                                       \n"
"    float4 Pos : POSITION;                                                              \n"
"    float2 Tex : TEXCOORD0;                                                             \n"
"};                                                                                      \n"
"//--------------------------------------------------------------------------------------\n"
"struct PS_INPUT                                                                         \n"
"{                                                                                       \n"
"    float4 Pos : SV_POSITION;                                                           \n"
"    float2 Tex : TEXCOORD0;                                                             \n"
"};                                                                                      \n"
"//--------------------------------------------------------------------------------------\n"
"// Vertex Shader                                                                        \n"
"//--------------------------------------------------------------------------------------\n"
"PS_INPUT VS( VS_INPUT input )                                                           \n"
"{                                                                                       \n"
"    PS_INPUT output = (PS_INPUT)0;                                                      \n"
"    output.Pos = mul( input.Pos, WorldViewProjection );                                 \n"
"    output.Tex = input.Tex;                                                             \n"
"                                                                                        \n"
"    return output;                                                                      \n"
"}                                                                                       \n"
"//--------------------------------------------------------------------------------------\n"
"// Pixel Shader passing texture color                                                   \n"
"//--------------------------------------------------------------------------------------\n"
"float4 PS( PS_INPUT input) : SV_Target                                                  \n"
"{                                                                                       \n"
"    return txDiffuse.Sample( samplerState, input.Tex );                                 \n"
"}                                                                                       \n"
;
return bColorCube ? s_pColorShaderText : s_pTextureShaderText;
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
ALVR_RESULT ConvertFromatFromDXGIToALVR(DXGI_FORMAT dxgi, ALVR_FORMAT &alvr)
{
    struct DxgiToAlvr
    {
        DXGI_FORMAT dxgi;
        ALVR_FORMAT alvr;
    };

static DxgiToAlvr s_DxgiToAlvr[] = 
{

    { DXGI_FORMAT_R32G32B32A32_TYPELESS ,            ALVR_FORMAT_R32G32B32A32_FLOAT },           
    { DXGI_FORMAT_R32G32B32A32_FLOAT ,               ALVR_FORMAT_R32G32B32A32_FLOAT },              
    { DXGI_FORMAT_R32G32B32A32_UINT ,                ALVR_FORMAT_R32G32B32A32_UINT },               
    { DXGI_FORMAT_R32G32B32A32_SINT ,                ALVR_FORMAT_R32G32B32A32_SINT },               
//    { DXGI_FORMAT_R32G32B32_TYPELESS ,               ALVR_FORMAT_R32G32B32_FLOAT },              
//    { DXGI_FORMAT_R32G32B32_FLOAT ,                  ALVR_FORMAT_R32G32B32_FLOAT },                 
//    { DXGI_FORMAT_R32G32B32_UINT ,                   ALVR_FORMAT_R32G32B32_UINT },                  
//    { DXGI_FORMAT_R32G32B32_SINT ,                   ALVR_FORMAT_R32G32B32_SINT },                  
    { DXGI_FORMAT_R16G16B16A16_TYPELESS ,            ALVR_FORMAT_R16G16B16A16_FLOAT },           
    { DXGI_FORMAT_R16G16B16A16_FLOAT ,               ALVR_FORMAT_R16G16B16A16_FLOAT },              
    { DXGI_FORMAT_R16G16B16A16_UNORM ,               ALVR_FORMAT_R16G16B16A16_UNORM },              
    { DXGI_FORMAT_R16G16B16A16_UINT ,                ALVR_FORMAT_R16G16B16A16_UINT },               
    { DXGI_FORMAT_R16G16B16A16_SNORM ,               ALVR_FORMAT_R16G16B16A16_SNORM },              
    { DXGI_FORMAT_R16G16B16A16_SINT ,                ALVR_FORMAT_R16G16B16A16_SINT },               
    { DXGI_FORMAT_R32G32_TYPELESS ,                  ALVR_FORMAT_R32G32_FLOAT },                 
    { DXGI_FORMAT_R32G32_FLOAT ,                     ALVR_FORMAT_R32G32_FLOAT },                    
    { DXGI_FORMAT_R32G32_UINT ,                      ALVR_FORMAT_R32G32_UINT },                     
    { DXGI_FORMAT_R32G32_SINT ,                      ALVR_FORMAT_R32G32_SINT },                     
//    { DXGI_FORMAT_R32G8X24_TYPELESS ,                ALVR_FORMAT_R32G8X24_TYPELESS },               
//    { DXGI_FORMAT_D32_FLOAT_S8X24_UINT ,             ALVR_FORMAT_D32_FLOAT_S8X24_UINT },            
//    { DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS ,         ALVR_FORMAT_R32_FLOAT_X8X24_TYPELESS },        
//    { DXGI_FORMAT_X32_TYPELESS_G8X24_UINT ,          ALVR_FORMAT_X32_TYPELESS_G8X24_UINT },         
//    { DXGI_FORMAT_R10G10B10A2_TYPELESS ,             ALVR_FORMAT_R10G10B10A2_TYPELESS },            
    { DXGI_FORMAT_R10G10B10A2_UNORM ,                ALVR_FORMAT_R10G10B10A2_UNORM },               
    { DXGI_FORMAT_R10G10B10A2_UINT ,                 ALVR_FORMAT_R10G10B10A2_UINT },                
    { DXGI_FORMAT_R11G11B10_FLOAT ,                  ALVR_FORMAT_R11G11B10_FLOAT },                 
    { DXGI_FORMAT_R8G8B8A8_TYPELESS ,                ALVR_FORMAT_R8G8B8A8_UNORM },               
    { DXGI_FORMAT_R8G8B8A8_UNORM ,                   ALVR_FORMAT_R8G8B8A8_UNORM },                  
    { DXGI_FORMAT_R8G8B8A8_UNORM_SRGB ,              ALVR_FORMAT_R8G8B8A8_UNORM_SRGB },             
    { DXGI_FORMAT_R8G8B8A8_UINT ,                    ALVR_FORMAT_R8G8B8A8_UINT },                   
    { DXGI_FORMAT_R8G8B8A8_SNORM ,                   ALVR_FORMAT_R8G8B8A8_SNORM },                  
    { DXGI_FORMAT_R8G8B8A8_SINT ,                    ALVR_FORMAT_R8G8B8A8_SINT },                   
    { DXGI_FORMAT_R16G16_TYPELESS ,                  ALVR_FORMAT_R16G16_FLOAT },                 
    { DXGI_FORMAT_R16G16_FLOAT ,                     ALVR_FORMAT_R16G16_FLOAT },                    
    { DXGI_FORMAT_R16G16_UNORM ,                     ALVR_FORMAT_R16G16_UNORM },                    
    { DXGI_FORMAT_R16G16_UINT ,                      ALVR_FORMAT_R16G16_UINT },                     
    { DXGI_FORMAT_R16G16_SNORM ,                     ALVR_FORMAT_R16G16_SNORM },                    
    { DXGI_FORMAT_R16G16_SINT ,                      ALVR_FORMAT_R16G16_SINT },                     
    { DXGI_FORMAT_R32_TYPELESS ,                     ALVR_FORMAT_R32_FLOAT },                    
    { DXGI_FORMAT_D32_FLOAT ,                        ALVR_FORMAT_R32_FLOAT },                       
    { DXGI_FORMAT_R32_FLOAT ,                        ALVR_FORMAT_R32_FLOAT },                       
    { DXGI_FORMAT_R32_UINT ,                         ALVR_FORMAT_R32_UINT },                        
    { DXGI_FORMAT_R32_SINT ,                         ALVR_FORMAT_R32_SINT },                        
//    { DXGI_FORMAT_R24G8_TYPELESS ,                   ALVR_FORMAT_R24G8_TYPELESS },                  
//    { DXGI_FORMAT_D24_UNORM_S8_UINT ,                ALVR_FORMAT_D24_UNORM_S8_UINT },               
//    { DXGI_FORMAT_R24_UNORM_X8_TYPELESS ,            ALVR_FORMAT_R24_UNORM_X8_TYPELESS },           
//    { DXGI_FORMAT_X24_TYPELESS_G8_UINT ,             ALVR_FORMAT_X24_TYPELESS_G8_UINT },            
    { DXGI_FORMAT_R8G8_TYPELESS ,                    ALVR_FORMAT_R8G8_UNORM },                   
    { DXGI_FORMAT_R8G8_UNORM ,                       ALVR_FORMAT_R8G8_UNORM },                      
    { DXGI_FORMAT_R8G8_UINT ,                        ALVR_FORMAT_R8G8_UINT },                       
    { DXGI_FORMAT_R8G8_SNORM ,                       ALVR_FORMAT_R8G8_SNORM },                      
    { DXGI_FORMAT_R8G8_SINT ,                        ALVR_FORMAT_R8G8_SINT },                       
    { DXGI_FORMAT_R16_TYPELESS                ,      ALVR_FORMAT_R16_FLOAT                },     
    { DXGI_FORMAT_R16_FLOAT                   ,      ALVR_FORMAT_R16_FLOAT                   },     
//    { DXGI_FORMAT_D16_UNORM                   ,      ALVR_FORMAT_D16_UNORM                   },     
    { DXGI_FORMAT_R16_UNORM                   ,      ALVR_FORMAT_R16_UNORM                   },     
    { DXGI_FORMAT_R16_UINT                    ,      ALVR_FORMAT_R16_UINT                    },     
    { DXGI_FORMAT_R16_SNORM                   ,      ALVR_FORMAT_R16_SNORM                   },     
    { DXGI_FORMAT_R16_SINT                    ,      ALVR_FORMAT_R16_SINT                    },     
    { DXGI_FORMAT_R8_TYPELESS                 ,      ALVR_FORMAT_R8_UNORM                 },     
    { DXGI_FORMAT_R8_UNORM                    ,      ALVR_FORMAT_R8_UNORM                    },     
    { DXGI_FORMAT_R8_UINT                     ,      ALVR_FORMAT_R8_UINT                     },     
    { DXGI_FORMAT_R8_SNORM                    ,      ALVR_FORMAT_R8_SNORM                    },     
    { DXGI_FORMAT_R8_SINT                     ,      ALVR_FORMAT_R8_SINT                     },     
//    { DXGI_FORMAT_A8_UNORM                    ,      ALVR_FORMAT_A8_UNORM                    },     
//    { DXGI_FORMAT_R1_UNORM                    ,      ALVR_FORMAT_R1_UNORM                    },     
//  { DXGI_FORMAT_R9G9B9E5_SHAREDEXP          ,      ALVR_FORMAT_R9G9B9E5_SHAREDEXP          },     
//  { DXGI_FORMAT_R8G8_B8G8_UNORM             ,      ALVR_FORMAT_R8G8_B8G8_UNORM             },     
//  { DXGI_FORMAT_G8R8_G8B8_UNORM             ,      ALVR_FORMAT_G8R8_G8B8_UNORM             },     
    { DXGI_FORMAT_BC1_TYPELESS                ,      ALVR_FORMAT_BC1_UNORM                },     
    { DXGI_FORMAT_BC1_UNORM                   ,      ALVR_FORMAT_BC1_UNORM                   },     
    { DXGI_FORMAT_BC1_UNORM_SRGB              ,      ALVR_FORMAT_BC1_UNORM_SRGB              },     
    { DXGI_FORMAT_BC2_TYPELESS                ,      ALVR_FORMAT_BC2_UNORM                },     
    { DXGI_FORMAT_BC2_UNORM                   ,      ALVR_FORMAT_BC2_UNORM                   },     
    { DXGI_FORMAT_BC2_UNORM_SRGB              ,      ALVR_FORMAT_BC2_UNORM_SRGB              },     
    { DXGI_FORMAT_BC3_TYPELESS                ,      ALVR_FORMAT_BC3_UNORM                },     
    { DXGI_FORMAT_BC3_UNORM                   ,      ALVR_FORMAT_BC3_UNORM                   },     
    { DXGI_FORMAT_BC3_UNORM_SRGB              ,      ALVR_FORMAT_BC3_UNORM_SRGB              },     
    { DXGI_FORMAT_BC4_TYPELESS                ,      ALVR_FORMAT_BC4_UNORM                },     
    { DXGI_FORMAT_BC4_UNORM                   ,      ALVR_FORMAT_BC4_UNORM                   },     
    { DXGI_FORMAT_BC4_SNORM                   ,      ALVR_FORMAT_BC4_SNORM                   },     
    { DXGI_FORMAT_BC5_TYPELESS                ,      ALVR_FORMAT_BC5_UNORM                },     
    { DXGI_FORMAT_BC5_UNORM                   ,      ALVR_FORMAT_BC5_UNORM                   },     
    { DXGI_FORMAT_BC5_SNORM                   ,      ALVR_FORMAT_BC5_SNORM                   },     
//  { DXGI_FORMAT_B5G6R5_UNORM                ,      ALVR_FORMAT_B5G6R5_UNORM                },     
//  { DXGI_FORMAT_B5G5R5A1_UNORM              ,      ALVR_FORMAT_B5G5R5A1_UNORM              },     
    { DXGI_FORMAT_B8G8R8A8_UNORM              ,      ALVR_FORMAT_B8G8R8A8_UNORM              },     
    { DXGI_FORMAT_B8G8R8X8_UNORM              ,      ALVR_FORMAT_B8G8R8X8_UNORM              },     
//  { DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM  ,      ALVR_FORMAT_R10G10B10_XR_BIAS_A2_UNORM  },     
    { DXGI_FORMAT_B8G8R8A8_TYPELESS           ,      ALVR_FORMAT_B8G8R8A8_UNORM_SRGB           },     
    { DXGI_FORMAT_B8G8R8A8_UNORM_SRGB         ,      ALVR_FORMAT_B8G8R8A8_UNORM_SRGB         },     
    { DXGI_FORMAT_B8G8R8X8_TYPELESS           ,      ALVR_FORMAT_B8G8R8X8_UNORM_SRGB           },     
    { DXGI_FORMAT_B8G8R8X8_UNORM_SRGB         ,      ALVR_FORMAT_B8G8R8X8_UNORM_SRGB         },     
    { DXGI_FORMAT_BC6H_TYPELESS               ,      ALVR_FORMAT_BC6H_UF16               },     
    { DXGI_FORMAT_BC6H_UF16                   ,      ALVR_FORMAT_BC6H_UF16                   },     
    { DXGI_FORMAT_BC6H_SF16                   ,      ALVR_FORMAT_BC6H_SF16                   },     
    { DXGI_FORMAT_BC7_TYPELESS                ,      ALVR_FORMAT_BC7_UNORM                },     
    { DXGI_FORMAT_BC7_UNORM                   ,      ALVR_FORMAT_BC7_UNORM                   },     
    { DXGI_FORMAT_BC7_UNORM_SRGB              ,      ALVR_FORMAT_BC7_UNORM_SRGB              },     

};

    alvr = ALVR_FORMAT_UNKNOWN;
    for (size_t i = 0; i < _countof(s_DxgiToAlvr); i++)
    {
        if (s_DxgiToAlvr[i].dxgi == dxgi)
        {
            alvr = s_DxgiToAlvr[i].alvr;
            return ALVR_OK;
        }
    }
    return  ALVR_FAIL;

}