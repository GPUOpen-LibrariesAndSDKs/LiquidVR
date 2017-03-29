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

#include <SDKDDKVer.h>

#include <stdio.h>
#include <tchar.h>
#include <conio.h>
#include "../../inc/LiquidVR.h"
#include "../common/inc/D3DHelper.h"
#include "../common/inc/LvrLogger.h"

//-------------------------------------------------------------------------------------------------
// Defines
//
#define AFFINITY_WORK_AROUND // remove all under this define when GXX fixes issue related to LvrResources + MGPU
#define MAKE_NV12_FILE // use to make output file of first frame for testing

//-------------------------------------------------------------------------------------------------
// Options
//
static const int                                g_iFrameCount = 100;
static const int                                g_iTextureWidthIn = 1296;
static const int                                g_iTextureHeightIn = 1440;
static const int                                g_iTextureWidthOut = 646;
static const int                                g_iTextureHeightOut = 720;
static const int								g_iFill = 0;
static const int								g_iFillY = 0;
static const int								g_iFillU = 0;
static const int								g_iFillV = 0;

//-------------------------------------------------------------------------------------------------
// Funcs
//
ALVR_RESULT	Init();
ALVR_RESULT RenderFrame();
ALVR_RESULT GetMotionVectors();
ALVR_RESULT BindAndFlipOutputs();
ALVR_RESULT	Terminate();
#if defined(MAKE_NV12_FILE)
ALVR_RESULT OutputFrame();
#endif

//-------------------------------------------------------------------------------------------------
// Structs
//
struct NV12Surface
{
    ATL::CComPtr<ALVRSurface> m_pParent;
    ATL::CComPtr<ALVRSurface> m_pY;
    ATL::CComPtr<ALVRSurface> m_pUV;
};

#pragma pack(push, 1)
struct GradientParameters
{
    int	            width;
    int	            height;
    int             frameNum;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct ConvertParameters
{
    int	            srcRect[4];
    int	            dstRect[4];
    unsigned int	fill;
    unsigned int	fillY;
    unsigned int	fillU;
    unsigned int	fillV;
};
#pragma pack(pop)

//-------------------------------------------------------------------------------------------------
// Vars
//
static D3DHelper								g_D3DHelper;

static ALVRFactory*								g_pFactory;
static ATL::CComPtr<ALVRDeviceExD3D11>          g_pLvrDevice;
static ATL::CComPtr<ALVRMotionEstimator>        g_pMotionEstimator;
static ATL::CComPtr<ALVRComputeContext>         g_pComputeContext;
static ATL::CComPtr<ALVRComputeTask>            g_pConvertTask;
static ATL::CComPtr<ALVRComputeTask>            g_pGradientTask;

static ATL::CComPtr<ALVRSurface>                g_pGradientSurf;
static ATL::CComPtr<ALVRBuffer>                 g_pGradientConstBuf;
static ATL::CComPtr<ALVRBuffer>                 g_pConvertConstBuf;
static ATL::CComPtr<ALVRSampler>                g_pSampler;

static NV12Surface                              g_OutputNV12[2];
static NV12Surface*                             g_pCurrOutputNV12 = NULL;
static NV12Surface*                             g_pPrevOutputNV12 = NULL;

#if defined(MAKE_NV12_FILE)
static ATL::CComPtr<ALVRBuffer>                 g_pOutputDataBuf[2];
#endif

static ATL::CComPtr<ALVRBuffer>                 g_pMotionVecBuf;

static ATL::CComPtr<ALVRFence>                  g_pFence;

#ifdef AFFINITY_WORK_AROUND
static ATL::CComPtr<ALVRGpuAffinity>            m_pLvrAffinity;
#endif

HMODULE											g_hLiquidVRDLL = NULL;

static const int                                g_iNumMacroBlocksY = g_iTextureWidthOut / 16;
static const int                                g_iNumMacroBlocksX = g_iTextureHeightOut / 16;
static int                                      g_iFrameNum = 0;

#if defined(MAKE_NV12_FILE)
static FILE*                                    g_pNV12File;
#endif

static const char s_BGRA_NV12_hlsl[] =
"#define LEVEL 16.f\n"
"#define DIV_VAL 256.f\n"

"SamplerState LinearSampler : register(s0)\n"
"{\n"
"    Filter = MIN_MAG_MIP_LINEAR;\n"
"    AddressU = Clamp;\n"
"    AddressV = Clamp;\n"
"};\n"

"Texture2D<float4>  inBGRA : register(t0);\n"
"RWTexture2D<uint>  outY   : register(u0);\n"
"RWTexture2D<uint2> outUV  : register(u1);\n"

"cbuffer Parameters : register(b0)\n"
"{\n"
"    int4 srcRect;\n"
"    int4 dstRect;\n"
"    uint fill;\n"
"    uint fillY;\n"
"    uint fillU;\n"
"    uint fillV;\n"
"};\n"

"void LoadQuad_BGRA(const int2 id, inout float4 R, inout float4 G, inout float4 B)\n"
"{\n"
"    float2 offset;\n"
"    offset.x = 1.f / float(srcRect.z);\n"
"    offset.y = 1.f / float(srcRect.w);\n"
"    float2 pos = (float2(id)+0.5f) * offset;\n"

"    float4 color = inBGRA.SampleLevel(LinearSampler, pos, 0) * 255.f;\n"
"    R.x = color.x;\n"
"    G.x = color.y;\n"
"    B.x = color.z;\n"

"    color = inBGRA.SampleLevel(LinearSampler, float2(pos.x + offset.x, pos.y), 0) * 255.f;\n"
"    R.y = color.x;\n"
"    G.y = color.y;\n"
"    B.y = color.z;\n"

"    color = inBGRA.SampleLevel(LinearSampler, float2(pos.x, pos.y + offset.y), 0) * 255.f;\n"
"    R.z = color.x;\n"
"    G.z = color.y;\n"
"    B.z = color.z;\n"

"    color = inBGRA.SampleLevel(LinearSampler, float2(pos.x + offset.x, pos.y + offset.y), 0) * 255.f;\n"
"    R.w = color.x;\n"
"    G.w = color.y;\n"
"    B.w = color.z;\n"
"}\n"

"    void StoreQuad_NV12(const int2 id, float4 Y, float U, float V)\n"
"    {\n"
"        int2 pos = id;\n"

"        outY[pos] = uint(Y.x);\n"
"        outY[int2(pos.x + 1, pos.y)] = uint(Y.y);\n"
"        outY[int2(pos.x, pos.y + 1)] = uint(Y.z);\n"
"        outY[int2(pos.x + 1, pos.y + 1)] = uint(Y.w);\n"

"        pos = id >> 1; // changed from just Y\n"

"        outUV[pos] = uint2(U, V);\n"

"        //outUV[pos] = uint(U);\n"
"        //outUV[pos + uint2(1, 0)] = uint(V);\n"
"    }\n"

"    float Average(float4 color)\n"
"    {\n"
"        return floor(dot(color, 1) * 0.25f + 0.5f);\n"
"    }\n"

"    void RGBtoYUV(inout float4 Y, inout float U, inout float V, float4 R, float4 G, float4 B)\n"
"    {\n"
"        Y = clamp((46.742f    *   R + 157.243f  *   G + 15.874f  * B + LEVEL) / DIV_VAL, 0.f, 255.f);\n"

"        float4 rgb = float4(Average(R), Average(G), Average(B), 0.f);\n"

"        U = clamp(((-25.765f) * rgb.x + (-86.674f)  * rgb.y + 112.439f   * rgb.z) / DIV_VAL + 128.5f, 0.f, 255.f);\n"
"        V = clamp((112.439f   * rgb.x + (-102.129f) * rgb.y + (-10.310f) * rgb.z) / DIV_VAL + 128.5f, 0.f, 255.f);\n"
"    }\n"

"    [numthreads(8, 8, 1)]\n"
"    void main(uint3 DTid : SV_DispatchThreadID)\n"
"    {\n"
"        int2 idOut = int2(DTid.x * 2, DTid.y * 2);\n"

"        if (idOut.x < dstRect.x || idOut.y < dstRect.y || idOut.x >= dstRect.z + dstRect.x || idOut.y >= dstRect.w + dstRect.y) {\n"
"            if (fill != 0)\n"
"            {\n"
"                StoreQuad_NV12(idOut, fillY, fillU, fillV);\n"
"            }\n"
"            return;\n"
"        }\n"
"        int inW = srcRect.z - srcRect.x;\n"
"        int inH = srcRect.w - srcRect.y;\n"
"        int outW = dstRect.z - dstRect.x;\n"
"        int outH = dstRect.w - dstRect.y;\n"
"        // to src coordinates to scale properly \n"
"        int2 idIn = int2((DTid.x - dstRect.x) * inW / outW  + srcRect.x, (DTid.y - dstRect.y) * inH / outH + srcRect.y);\n"

"        float4 Y;\n"
"        float U, V;\n"
"        float4 R, G, B;\n"

"        LoadQuad_BGRA(idIn * 2, R, G, B);\n"
"        RGBtoYUV(Y, U, V, R, G, B);\n"
"        StoreQuad_NV12(idOut, Y, U, V);\n"
"    }";

static const char s_MotionGradient_hlsl[] =
"#define PI 3.14159265f\n"

"RWTexture2D<float4> outImage : register(u0);\n"

"cbuffer Parameters : register(b0)\n"
"{\n"
"    uint width;\n"
"    uint height;\n"
"    uint frameNum;\n"
"};\n"

"[numthreads(8, 8, 1)]\n"
"void main(uint3 DTid : SV_DispatchThreadID)\n"
"{\n"
"    if ((DTid.x < width) && (DTid.y < height))\n"
"    {\n"
"        float sineArg = (float)frameNum + (float)DTid.x * (2.0f * PI / width);\n"
"        float r = (sin(sineArg) + 1.0f) * 0.5f;\n"
"        outImage[DTid.xy] = float4(r, 0.f, 0.f, 0.f);\n"
"    }\n"
"}\n";


//-------------------------------------------------------------------------------------------------
// Main
//
int _tmain(int argc, _TCHAR* argv[])
{
    ALVR_RESULT res = ALVR_OK;

    res = Init();
    if (res == ALVR_OK)
    {
#if defined(MAKE_NV12_FILE)
        wchar_t fileName[50];
        swprintf_s(fileName, L"../convert_output_%dx%d.nv12.yuv", g_iTextureWidthOut, g_iTextureHeightOut);
        _wfopen_s(&g_pNV12File, fileName, L"wb");
#endif
        while (g_iFrameNum < g_iFrameCount)
        {
            BindAndFlipOutputs();
            RenderFrame();

#if defined(MAKE_NV12_FILE)
            OutputFrame();
#endif

            if (g_iFrameNum > 0) // only get motion vectors after first frame, in order to submit 2 frames to compare
            {
                // do stuff with motion vectors here
                GetMotionVectors();
            }

            g_iFrameNum++;

            printf("Rendered %i frames\n", g_iFrameNum);

            Sleep(0);
        }
#if defined(MAKE_NV12_FILE)
        fclose(g_pNV12File);
#endif
    }

    Terminate();
    return res;
}

//-------------------------------------------------------------------------------------------------
ALVR_RESULT Init()
{
    HRESULT hr = S_OK;

    //---------------------------------------------------------------------------------------------
    // Init ALVR
    //---------------------------------------------------------------------------------------------
    ALVR_RESULT res = ALVR_OK;
    g_hLiquidVRDLL = LoadLibraryW(ALVR_DLL_NAME);
    CHECK_RETURN(g_hLiquidVRDLL != NULL, ALVR_FAIL, L"DLL " << ALVR_DLL_NAME << L" is not found");

    ALVRInit_Fn pFunInit = (ALVRInit_Fn)GetProcAddress(g_hLiquidVRDLL, ALVR_INIT_FUNCTION_NAME);
    res = pFunInit(ALVR_FULL_VERSION, (void**)&g_pFactory);
    CHECK_ALVR_ERROR_RETURN(res, ALVR_INIT_FUNCTION_NAME << L"failed");

#if defined(AFFINITY_WORK_AROUND)
    res = g_pFactory->CreateGpuAffinity(&m_pLvrAffinity);
    CHECK_ALVR_ERROR_RETURN(res, L"CreateGpuAffinity() failed");

    res = m_pLvrAffinity->EnableGpuAffinity(ALVR_GPU_AFFINITY_FLAGS_NONE);
    CHECK_ALVR_ERROR_RETURN(res, L"EnableGpuAffinity() failed");

    res = res = m_pLvrAffinity->DisableGpuAffinity();
    CHECK_ALVR_ERROR_RETURN(res, L"DisableGpuAffinity() failed");
#endif

    //---------------------------------------------------------------------------------------------
    // Create D3D11 Device
    //---------------------------------------------------------------------------------------------
    res = g_D3DHelper.CreateD3D11Device();
    CHECK_ALVR_ERROR_RETURN(res, L"CreateD3D11Device() failed");

    //---------------------------------------------------------------------------------------------
    // Create ALVR device
    //---------------------------------------------------------------------------------------------
    res = g_pFactory->CreateALVRDeviceExD3D11(g_D3DHelper.m_pd3dDevice, NULL, &g_pLvrDevice);
    CHECK_ALVR_ERROR_RETURN(res, L"CreateALVRDeviceExD3D11() failed");

    //---------------------------------------------------------------------------------------------
    // Create Motion Estimator
    //---------------------------------------------------------------------------------------------
    ALVRMotionEstimatorConfig meConf;
    ZeroMemory(&meConf, sizeof(meConf));
    meConf.width = g_iTextureWidthOut;
    meConf.height = g_iTextureHeightOut;

    res = g_pFactory->CreateMotionEstimator(g_pLvrDevice, &meConf, &g_pMotionEstimator);
    CHECK_ALVR_ERROR_RETURN(res, L"CreateMotionEstimator() failed");

    ALVRMotionEstimatorDesc meDesc;
    res = g_pMotionEstimator->GetDesc(&meDesc);
    CHECK_ALVR_ERROR_RETURN(res, L"MotionEstimator->GetDesc() failed");
    CHECK_RETURN(g_iTextureWidthOut <= meDesc.maxWidthInPixels && g_iTextureHeightOut <= meDesc.maxHeightInPixels, ALVR_FAIL,
        L"Bounds of image (" << g_iTextureWidthOut << L"x" << g_iTextureHeightOut << L") " <<
        L"unsupported by Motion Estimator (max: " << meDesc.maxWidthInPixels << L"x" << meDesc.maxHeightInPixels << L")");

    g_pMotionEstimator->Open(&meConf);

    //---------------------------------------------------------------------------------------------
    // Create ASync Compute Context
    //---------------------------------------------------------------------------------------------
    ALVRComputeContextDesc computeDesc;
    ZeroMemory(&computeDesc, sizeof(computeDesc));
    computeDesc.flags = ALVR_COMPUTE_NONE;

    res = g_pFactory->CreateComputeContext(g_pLvrDevice, 0, &computeDesc, &g_pComputeContext);
    CHECK_ALVR_ERROR_RETURN(res, L"CreateComputeContext() failed");

    //---------------------------------------------------------------------------------------------
    // Compile Shaders
    //---------------------------------------------------------------------------------------------
    ATL::CComPtr<ID3DBlob> pVSBlob;
    ATL::CComPtr<ID3DBlob> pErrorBlob;

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;

    // Compile gradient shader
    hr = D3DCompile(s_MotionGradient_hlsl, strlen(s_MotionGradient_hlsl), NULL, NULL, NULL, "main", "cs_5_0", dwShaderFlags, 0, &pVSBlob, &pErrorBlob);
    CHECK_HRESULT_ERROR_RETURN(hr, L"D3DCompile(CS, MotionGradient) failed" << reinterpret_cast<char*>(pErrorBlob->GetBufferPointer()));

    res = g_pComputeContext->CreateComputeTask(ALVR_SHADER_MODEL_D3D11, 0, pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &g_pGradientTask);
    CHECK_ALVR_ERROR_RETURN(res, L"CreateComputeTask(MotionGradientShader) failed");

    pVSBlob.Release();
    pErrorBlob.Release();

    // Compile convert shader
    hr = D3DCompile(s_BGRA_NV12_hlsl, strlen(s_BGRA_NV12_hlsl), NULL, NULL, NULL, "main", "cs_5_0", dwShaderFlags, 0, &pVSBlob, &pErrorBlob);
    CHECK_HRESULT_ERROR_RETURN(hr, L"D3DCompile(CS, BGRA_NV12) failed" << reinterpret_cast<char*>(pErrorBlob->GetBufferPointer()));

    res = g_pComputeContext->CreateComputeTask(ALVR_SHADER_MODEL_D3D11, 0, pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &g_pConvertTask);
    CHECK_ALVR_ERROR_RETURN(res, L"CreateComputeTask(ConvertShader) failed");

    pVSBlob.Release();
    pErrorBlob.Release();

    //---------------------------------------------------------------------------------------------
    // Gradient Buffer
    //---------------------------------------------------------------------------------------------
    ALVRBufferDesc buffDesc;
    ZeroMemory(&buffDesc, sizeof(buffDesc));
    buffDesc.size = sizeof(GradientParameters);
    buffDesc.apiSupport = ALVR_RESOURCE_API_ASYNC_COMPUTE;
    buffDesc.bufferFlags = ALVR_BUFFER_CONSTANT;
    buffDesc.cpuAccessFlags = ALVR_CPU_ACCESS_WRITE;
    buffDesc.structureStride = 0;

    res = g_pComputeContext->CreateBuffer(&buffDesc, &g_pGradientConstBuf);
    CHECK_ALVR_ERROR_RETURN(res, L"CreateBuffer(GradientConstBuf) failed");

    res = g_pGradientTask->BindConstantBuffer(0, g_pGradientConstBuf);
    CHECK_ALVR_ERROR_RETURN(res, L"BindConstantBuffer(GradientConstBuf) failed");
    // Fill this buffer each frame instead of at init

    //---------------------------------------------------------------------------------------------
    // Create Gradient shader output 
    //---------------------------------------------------------------------------------------------
    // Input surface BGRA
    ALVRSurfaceDesc surfDesc;
    ZeroMemory(&surfDesc, sizeof(surfDesc));
    surfDesc.type = ALVR_SURFACE_2D;
    surfDesc.surfaceFlags = ALVR_SURFACE_SHADER_INPUT | ALVR_SURFACE_SHADER_OUTPUT;
    surfDesc.apiSupport = ALVR_RESOURCE_API_ASYNC_COMPUTE | ALVR_RESOURCE_API_D3D11;
    surfDesc.width = g_iTextureWidthIn;
    surfDesc.height = g_iTextureHeightIn;
    surfDesc.format = ALVR_FORMAT_B8G8R8A8_UNORM;
    surfDesc.sliceCount = 1;
    surfDesc.mipCount = 1;

    res = g_pComputeContext->CreateSurface(&surfDesc, &g_pGradientSurf);
    CHECK_ALVR_ERROR_RETURN(res, L"CreateSurface(GradientSurf) failed");

    res = g_pGradientTask->BindOutput(0, g_pGradientSurf);
    CHECK_ALVR_ERROR_RETURN(res, L"GradientTask->BindOutput(GradientSurf) failed");

    res = g_pConvertTask->BindInput(0, g_pGradientSurf); // bind as input to next shader
    CHECK_ALVR_ERROR_RETURN(res, L"ConvertTask->BindInput(GradientSurf) failed");

    //---------------------------------------------------------------------------------------------
    // Convert Sampler
    //---------------------------------------------------------------------------------------------
    ALVRSamplerDesc samplerDesc;
    ZeroMemory(&samplerDesc, sizeof(samplerDesc));
    samplerDesc.filterMode = ALVR_FILTER_POINT;
    samplerDesc.addressU = ALVR_ADDRESS_CLAMP;
    samplerDesc.addressV = ALVR_ADDRESS_CLAMP;
    samplerDesc.addressW = ALVR_ADDRESS_CLAMP;
    samplerDesc.maxAnisotropy = 1;
    samplerDesc.borderColorType = ALVR_BORDER_COLOR_OPAQUE_BLACK;

    res = g_pComputeContext->CreateSampler(&samplerDesc, &g_pSampler);
    CHECK_ALVR_ERROR_RETURN(res, L"CreateSampler(Sampler) failed");

    res = g_pConvertTask->BindSampler(0, g_pSampler);
    CHECK_ALVR_ERROR_RETURN(res, L"ConvertTask->BindSampler(Sampler) failed");

    //---------------------------------------------------------------------------------------------
    // Convert Buffer
    //--------------------------------------------------------------------------------------------
    ZeroMemory(&buffDesc, sizeof(buffDesc));
    buffDesc.size = sizeof(ConvertParameters);
    buffDesc.apiSupport = ALVR_RESOURCE_API_ASYNC_COMPUTE;
    buffDesc.bufferFlags = ALVR_BUFFER_CONSTANT;
    buffDesc.cpuAccessFlags = ALVR_CPU_ACCESS_WRITE;

    res = g_pComputeContext->CreateBuffer(&buffDesc, &g_pConvertConstBuf);
    CHECK_ALVR_ERROR_RETURN(res, L"CreateBuffer(ConvertConstBuf) failed");

    res = g_pConvertTask->BindConstantBuffer(0, g_pConvertConstBuf);
    CHECK_ALVR_ERROR_RETURN(res, L"BindConstantBuffer(ConvertConstBuf) failed");

    ConvertParameters cParams;
    ZeroMemory(&cParams, sizeof(cParams));
    cParams.fill = g_iFill;
    cParams.fillY = g_iFillY;
    cParams.fillU = g_iFillU;
    cParams.fillV = g_iFillV;
    cParams.srcRect[0] = 0; cParams.srcRect[1] = 0; cParams.srcRect[2] = g_iTextureWidthIn; cParams.srcRect[3] = g_iTextureHeightIn;
    cParams.dstRect[0] = 0; cParams.dstRect[1] = 0; cParams.dstRect[2] = g_iTextureWidthOut; cParams.dstRect[3] = g_iTextureHeightOut;

    void* pBuffPtr = NULL;
    res = g_pConvertConstBuf->Map(&pBuffPtr);
    CHECK_ALVR_ERROR_RETURN(res, L"Map(ConvertConstBuf) failed");
    memcpy(pBuffPtr, &cParams, sizeof(cParams));
    res = g_pConvertConstBuf->Unmap();
    CHECK_ALVR_ERROR_RETURN(res, L"Unmap(ConvertConstBuf) failed");

    //---------------------------------------------------------------------------------------------
    // Create Convert shader output 
    //---------------------------------------------------------------------------------------------
    ZeroMemory(&surfDesc, sizeof(surfDesc));
    surfDesc.type = ALVR_SURFACE_2D;
    surfDesc.surfaceFlags = ALVR_SURFACE_SHADER_OUTPUT | ALVR_SURFACE_SHADER_INPUT;
    surfDesc.apiSupport = ALVR_RESOURCE_API_ASYNC_COMPUTE | ALVR_RESOURCE_API_D3D11;
    surfDesc.width = g_iTextureWidthOut;
    surfDesc.height = g_iTextureHeightOut;
    surfDesc.format = ALVR_FORMAT_NV12;
    surfDesc.sliceCount = 1;
    surfDesc.mipCount = 1;

    res = g_pComputeContext->CreateSurface(&surfDesc, &g_OutputNV12[0].m_pParent); // Create group surface
    CHECK_ALVR_ERROR_RETURN(res, L"CreateSurface(OutputParentSurf 0) failed");

    res = g_pComputeContext->CreateSurface(&surfDesc, &g_OutputNV12[1].m_pParent); // Create another one for flip
    CHECK_ALVR_ERROR_RETURN(res, L"CreateSurface(OutputParentSurf 1) failed");

    ALVRChildSurfaceDesc childSurfDesc;
    ZeroMemory(&childSurfDesc, sizeof(childSurfDesc));
    childSurfDesc.shaderOutputFormat = ALVR_FORMAT_R8_UINT;

    res = g_pComputeContext->CreateChildSurface(g_OutputNV12[0].m_pParent, &childSurfDesc, &g_OutputNV12[0].m_pY); // Create Y' child surface
    CHECK_ALVR_ERROR_RETURN(res, L"CreateChildSurface(OutputNV12_0->Y) failed");

    res = g_pComputeContext->CreateChildSurface(g_OutputNV12[1].m_pParent, &childSurfDesc, &g_OutputNV12[1].m_pY); // Create Y' child surface for flip
    CHECK_ALVR_ERROR_RETURN(res, L"CreateChildSurface(OutputNV12_1->Y) failed");

    childSurfDesc.shaderOutputFormat = ALVR_FORMAT_R8G8_UINT;

    res = g_pComputeContext->CreateChildSurface(g_OutputNV12[0].m_pParent, &childSurfDesc, &g_OutputNV12[0].m_pUV); // Create UV child surface
    CHECK_ALVR_ERROR_RETURN(res, L"CreateChildSurface(OutputNV12_0->UV) failed");

    res = g_pComputeContext->CreateChildSurface(g_OutputNV12[1].m_pParent, &childSurfDesc, &g_OutputNV12[1].m_pUV); // Create UV child surface for flip
    CHECK_ALVR_ERROR_RETURN(res, L"CreateChildSurface(OutputNV12_1->UV) failed");

    //---------------------------------------------------------------------------------------------
    // Output motion vectors buffer
    //---------------------------------------------------------------------------------------------
    ZeroMemory(&buffDesc, sizeof(buffDesc));
    buffDesc.size = sizeof(ALVRMotionVectorForMB) * g_iNumMacroBlocksX * g_iNumMacroBlocksY;
    buffDesc.apiSupport = ALVR_RESOURCE_API_ASYNC_COMPUTE | ALVR_RESOURCE_API_D3D11 | ALVR_RENDER_API_MOTION_ESTIMATOR;
    buffDesc.cpuAccessFlags = ALVR_CPU_ACCESS_READ;
    buffDesc.bufferFlags = ALVR_BUFFER_CONSTANT;

    res = g_pComputeContext->CreateBuffer(&buffDesc, &g_pMotionVecBuf);
    CHECK_ALVR_ERROR_RETURN(res, L"CreateBuffer(MotionVecBuf) failed");

#if defined(MAKE_NV12_FILE)
    //---------------------------------------------------------------------------------------------
    // Make output buffers that can be read from
    //---------------------------------------------------------------------------------------------s
    // Make a buffer to store Y' output
    memset(&buffDesc, 0, sizeof(buffDesc));
    buffDesc.size = g_iTextureWidthOut * g_iTextureHeightOut;
    buffDesc.apiSupport = ALVR_RESOURCE_API_ASYNC_COMPUTE;
    buffDesc.cpuAccessFlags = ALVR_CPU_ACCESS_READ;
    buffDesc.structureStride = g_iTextureWidthOut;
    buffDesc.format = ALVR_FORMAT_R8_UINT;

    res = g_pComputeContext->CreateBuffer(&buffDesc, &g_pOutputDataBuf[0]);
    CHECK_ALVR_ERROR_RETURN(res, L"CreateBuffer(OutputDataBuf 0) failed");

    // Make a buffer to store UV' output
    buffDesc.size = (g_iTextureWidthOut * g_iTextureHeightOut) / 2;
    buffDesc.format = ALVR_FORMAT_R8G8_UINT;

    res = g_pComputeContext->CreateBuffer(&buffDesc, &g_pOutputDataBuf[1]);
    CHECK_ALVR_ERROR_RETURN(res, L"CreateBuffer(OutputDataBuf 1) failed");
#endif // MAKE_NV12_FILE

    //---------------------------------------------------------------------------------------------
    // Make sync fence
    //---------------------------------------------------------------------------------------------
    res = g_pLvrDevice->CreateFence(&g_pFence);
    CHECK_ALVR_ERROR_RETURN(res, L"CreateFence() failed");

    return ALVR_OK;
}
//-------------------------------------------------------------------------------------------------
ALVR_RESULT RenderFrame()
{
    ALVR_RESULT res = ALVR_OK;

    // Fill in gradient parameters each frame (to use FrameNum to move the gradient)
    GradientParameters gParams;
    ZeroMemory(&gParams, sizeof(gParams));
    gParams.width = g_iTextureWidthIn;
    gParams.height = g_iTextureHeightIn;
    gParams.frameNum = g_iFrameNum;

    void* pBuffPtr = NULL;
    res = g_pGradientConstBuf->Map(&pBuffPtr);
    CHECK_ALVR_ERROR_RETURN(res, L"Map(GradientConstBuf) failed");
    memcpy(pBuffPtr, &gParams, sizeof(gParams));
    res = g_pGradientConstBuf->Unmap();
    CHECK_ALVR_ERROR_RETURN(res, L"Unmap(GradientConstBuf) failed");

    // Queue tasks
    static const int THREAD_GROUP_SIZE = 8;

    ALVRPoint3D offset = { 0, 0, 0 };
    ALVRSize3D sizeGradient = { (g_iTextureWidthIn + THREAD_GROUP_SIZE - 1) / THREAD_GROUP_SIZE, (g_iTextureHeightIn + THREAD_GROUP_SIZE - 1) / THREAD_GROUP_SIZE, 0 };

    res = g_pComputeContext->QueueTask(g_pGradientTask, &offset, &sizeGradient);
    CHECK_ALVR_ERROR_RETURN(res, L"QueueTask(Gradient) failed");

    ALVRSize3D sizeConvert = { (g_iTextureWidthOut + THREAD_GROUP_SIZE - 1) / THREAD_GROUP_SIZE, (g_iTextureHeightOut + THREAD_GROUP_SIZE - 1) / THREAD_GROUP_SIZE, 0 };
    res = g_pComputeContext->QueueTask(g_pConvertTask, &offset, &sizeConvert);
    CHECK_ALVR_ERROR_RETURN(res, L"QueueTask(Convert) failed");

    // Run queue
    res = g_pComputeContext->Flush(g_pFence);
    CHECK_ALVR_ERROR_RETURN(res, L"Flush() failed");
    res = g_pFence->Wait(2000);
    CHECK_ALVR_ERROR_RETURN(res, L"Fence->Wait() failed");

    return res;
}
//-------------------------------------------------------------------------------------------------
#if defined(MAKE_NV12_FILE)
ALVR_RESULT OutputFrame()
{
    ALVR_RESULT res = ALVR_OK;

    // Queue copying frame to buffers to be outputted to a file
    res = g_pComputeContext->QueueCopySurfaceToBuffer(g_pCurrOutputNV12->m_pY, NULL, g_pOutputDataBuf[0], 0);
    CHECK_ALVR_ERROR_RETURN(res, L"QueueCopyBufferToSurface(CurrOutputNV12->Y, OutputDataBuf 0) failed");
    res = g_pComputeContext->QueueCopySurfaceToBuffer(g_pCurrOutputNV12->m_pUV, NULL, g_pOutputDataBuf[1], 0);
    CHECK_ALVR_ERROR_RETURN(res, L"QueueCopyBufferToSurface(CurrOutputNV12->UV, OutputDataBuf 1) failed");

    // Run queue
    res = g_pComputeContext->Flush(g_pFence);
    CHECK_ALVR_ERROR_RETURN(res, L"Flush() failed");
    res = g_pFence->Wait(2000);
    CHECK_ALVR_ERROR_RETURN(res, L"Fence->Wait() failed");

    //---------------------------------------------------------------------------------------------
    // Make output file
    //---------------------------------------------------------------------------------------------
    void* pBuffPtr = NULL;
    // from Y' surface
    {
        res = g_pOutputDataBuf[0]->Map(&pBuffPtr);
        CHECK_ALVR_ERROR_RETURN(res, L"Map(OutputDataBuf 0) failed");

        uint8_t* data = static_cast<uint8_t*>(pBuffPtr);
        if (data != NULL) {
            int pixelSize = 1;
            int width = g_iTextureWidthOut;
            int height = g_iTextureHeightOut;
            size_t pitchH = g_iTextureWidthOut;

            for (int y = 0; y < height; ++y) {
                uint8_t* line = data + y * pitchH;
                fwrite(line, pixelSize, width, g_pNV12File);
            }

            LOG_INFO(L"Wrote Y' data");
        }

        res = g_pOutputDataBuf[0]->Unmap();
        CHECK_ALVR_ERROR_RETURN(res, L"Unmap(OutputDataBuf 0) failed");
    }
    pBuffPtr = NULL;
    // from UV surface
    {
        res = g_pOutputDataBuf[1]->Map(&pBuffPtr);
        CHECK_ALVR_ERROR_RETURN(res, L"Map(OutputDataBuf 1) failed");

        uint8_t* data = static_cast<uint8_t*>(pBuffPtr);
        if (data != NULL) {
            int pixelSize = 2;
            int width = g_iTextureWidthOut / 2;
            int height = g_iTextureHeightOut / 2;
            size_t pitchH = g_iTextureWidthOut;

            for (int y = 0; y < height; ++y) {
                uint8_t* line = data + y * pitchH;
                fwrite(line, pixelSize, width, g_pNV12File);
            }

            LOG_INFO(L"Wrote UV data");
        }

        res = g_pOutputDataBuf[1]->Unmap();
        CHECK_ALVR_ERROR_RETURN(res, L"Unmap(OutputDataBuf 1) failed");
    }

    return res;
}
#endif // MAKE_NV12_FILE
//-------------------------------------------------------------------------------------------------
ALVR_RESULT GetMotionVectors()
{
    ALVR_RESULT res = ALVR_OK;

    res = g_pMotionEstimator->SubmitFrames(g_pCurrOutputNV12->m_pParent, g_pPrevOutputNV12->m_pParent, g_pMotionVecBuf, NULL, NULL);
    CHECK_ALVR_ERROR_RETURN(res, L"SubmitFrames() failed");

    res = g_pMotionEstimator->QueryMotionVectors(NULL);
    CHECK_ALVR_ERROR_RETURN(res, L"QueryMotionVectors() failed");

    void* pBuffPtr = NULL;
    res = g_pMotionVecBuf->Map(&pBuffPtr);
    CHECK_ALVR_ERROR_RETURN(res, L"Map(MotionVecBuf) failed");

    // do stuff with motion vectors here

    res = g_pMotionVecBuf->Unmap();
    CHECK_ALVR_ERROR_RETURN(res, L"Unmap(MotionVecBuf) failed");

    return res;
}
//-------------------------------------------------------------------------------------------------
ALVR_RESULT BindAndFlipOutputs()
{
    ALVR_RESULT res = ALVR_OK;

    // first frame current will be 0 and previous 1, 
    // 0 will have data, 1 will not
    // first frame is not submitted to motion vectors until second frame
    // at which time the first frame will be 1 (prev), and the second frame will be 0 (cur)
    static int cur = 0;
    g_pCurrOutputNV12 = &g_OutputNV12[cur];
    cur = cur == 0 ? 1 : 0;
    g_pPrevOutputNV12 = &g_OutputNV12[cur]; // when current is 0, prev is 1, and vice versa

    res = g_pConvertTask->BindOutput(0, g_pCurrOutputNV12->m_pY);
    CHECK_ALVR_ERROR_RETURN(res, L"ConvertTask->BindOutput(CurrentOutputNV12->Y) failed");
    res = g_pConvertTask->BindOutput(1, g_pCurrOutputNV12->m_pUV);
    CHECK_ALVR_ERROR_RETURN(res, L"ConvertTask->BindOutput(CurrentOutputNV12->UV) failed");

    return res;
}
//-------------------------------------------------------------------------------------------------
ALVR_RESULT Terminate()
{
    g_pConvertTask.Release();
    g_pGradientTask.Release();

    g_pGradientConstBuf.Release();
    g_pConvertConstBuf.Release();
    g_pSampler.Release();
    g_pGradientSurf.Release();
    g_pMotionVecBuf.Release();

#if defined(MAKE_NV12_FILE)
    for (int i = 0; i < _countof(g_pOutputDataBuf); i++)
    {
        g_pOutputDataBuf[i].Release();
    }
#endif

    for (int i = 0; i < _countof(g_OutputNV12); i++)
    {
        g_OutputNV12[i].m_pY.Release();
        g_OutputNV12[i].m_pUV.Release();
        g_OutputNV12[i].m_pParent.Release();
    }
    g_pCurrOutputNV12 = NULL;
    g_pPrevOutputNV12 = NULL;

    g_pFence.Release();

    g_pComputeContext.Release();

    if (g_pMotionEstimator != nullptr)
    {
        g_pMotionEstimator->Close();
        g_pMotionEstimator.Release();
    }

    g_pLvrDevice.Release();

    g_D3DHelper.Terminate();

#if defined(AFFINITY_WORK_AROUND)
    m_pLvrAffinity.Release();
#endif
    g_pFactory = NULL;

    ::FreeLibrary(g_hLiquidVRDLL);

    return ALVR_OK;
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
