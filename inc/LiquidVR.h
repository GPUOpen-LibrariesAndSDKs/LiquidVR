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

#include <D3D11.h>
#include <cstdint>

#define ALVR_VERSION_MAJOR          1
#define ALVR_VERSION_MINOR          0
#define ALVR_VERSION_RELEASE        3
#define ALVR_VERSION_BUILD          20

#define ALVR_FULL_VERSION ( (uint64_t(ALVR_VERSION_MAJOR) << 48ull) | (uint64_t(ALVR_VERSION_MINOR) << 32ull) | (uint64_t(ALVR_VERSION_RELEASE) << 16ull)  | uint64_t(ALVR_VERSION_BUILD))

#define ALVR_STD_CALL               __stdcall
#define ALVR_CDECL_CALL             __cdecl
#define ALVR_NO_VTABLE              __declspec(novtable)

#define ALVR_INTERFACE(x) struct DECLSPEC_UUID(x) ALVR_NO_VTABLE

/**
***************************************************************************************************
* @brief error codes
***************************************************************************************************
*/
enum ALVR_RESULT
{
    ALVR_OK                         = 0,
    ALVR_FALSE                      = 1,
    ALVR_FAIL                       = 2,
    ALVR_INVALID_ARGUMENT           = 3,
    ALVR_NOT_INITIALIZED            = 4,
    ALVR_INSUFFICIENT_BUFFER_SIZE   = 5,
    ALVR_NOT_IMPLEMENTED            = 6,
    ALVR_NULL_POINTER               = 7,
    ALVR_ALREADY_INITIALIZED        = 8,
    ALVR_UNSUPPORTED_VERSION        = 9,
    ALVR_OUT_OF_MEMORY              = 10,
    ALVR_DISPLAY_REMOVED            = 11,
    ALVR_DISPLAY_USED               = 12,
    ALVR_DISPLAY_UNAVAILABLE        = 13,
    ALVR_DISPLAY_NOT_ENABLED        = 14,
    ALVR_OUTSTANDING_PRESENT_FRAME  = 15,
    ALVR_DEVICE_LOST                = 16,
    ALVR_UNAVAILABLE                = 17,
    ALVR_NOT_READY                  = 18,
    ALVR_TIMEOUT                    = 19,
    ALVR_RESOURCE_IS_NOT_BOUND      = 20,
    ALVR_UNSUPPORTED                = 21,
    ALVR_INCOMPATIBLE_DRIVER        = 22,
    ALVR_DEVICE_MISMATCH            = 23,
};

/**
***************************************************************************************************
* @brief transfer engines
***************************************************************************************************
*/
enum ALVR_GPU_ENGINE
{
    ALVR_GPU_ENGINE_3D              = 0,
    ALVR_GPU_ENGINE_DMA             = 1,
};

/**
***************************************************************************************************
* @brief interop APIs
***************************************************************************************************
*/
enum ALVR_RENDER_API
{
    ALVR_RENDER_API_ASYNC_COMPUTE   = 0,
    ALVR_RENDER_API_D3D11           = 1,
};

/**
***************************************************************************************************
* @brief ALVR formats
***************************************************************************************************
*/

enum ALVR_FORMAT
{
    ALVR_FORMAT_UNKNOWN = 0,                                    // DXGI_FORMAT_UNKNOWN = 0,
    ALVR_FORMAT_R32G32B32A32_FLOAT = 2,                         // DXGI_FORMAT_R32G32B32A32_FLOAT = 2,
    ALVR_FORMAT_R32G32B32A32_UINT = 3,                          // DXGI_FORMAT_R32G32B32A32_UINT = 3,
    ALVR_FORMAT_R32G32B32A32_SINT = 4,                          // DXGI_FORMAT_R32G32B32A32_SINT = 4,
    ALVR_FORMAT_R16G16B16A16_FLOAT = 10,                        // DXGI_FORMAT_R16G16B16A16_FLOAT = 10,
    ALVR_FORMAT_R16G16B16A16_UNORM = 11,                        // DXGI_FORMAT_R16G16B16A16_UNORM = 11,
    ALVR_FORMAT_R16G16B16A16_UINT = 12,                         // DXGI_FORMAT_R16G16B16A16_UINT = 12,
    ALVR_FORMAT_R16G16B16A16_SNORM = 13,                        // DXGI_FORMAT_R16G16B16A16_SNORM = 13,
    ALVR_FORMAT_R16G16B16A16_SINT = 14,                         // DXGI_FORMAT_R16G16B16A16_SINT = 14,
    ALVR_FORMAT_R32G32_FLOAT = 16,                              // DXGI_FORMAT_R32G32_FLOAT = 16,
    ALVR_FORMAT_R32G32_UINT = 17,                               // DXGI_FORMAT_R32G32_UINT = 17,
    ALVR_FORMAT_R32G32_SINT = 18,                               // DXGI_FORMAT_R32G32_SINT = 18,
    ALVR_FORMAT_R10G10B10A2_UNORM = 24,                         // DXGI_FORMAT_R10G10B10A2_UNORM = 24,
    ALVR_FORMAT_R10G10B10A2_UINT = 25,                          // DXGI_FORMAT_R10G10B10A2_UINT = 25,
    ALVR_FORMAT_R11G11B10_FLOAT = 26,                           // DXGI_FORMAT_R11G11B10_FLOAT = 26,
    ALVR_FORMAT_R8G8B8A8_UNORM = 28,                            // DXGI_FORMAT_R8G8B8A8_UNORM = 28,
    ALVR_FORMAT_R8G8B8A8_UNORM_SRGB = 29,                       // DXGI_FORMAT_R8G8B8A8_UNORM_SRGB = 29,
    ALVR_FORMAT_R8G8B8A8_UINT = 30,                             // DXGI_FORMAT_R8G8B8A8_UINT = 30,
    ALVR_FORMAT_R8G8B8A8_SNORM = 31,                            // DXGI_FORMAT_R8G8B8A8_SNORM = 31,
    ALVR_FORMAT_R8G8B8A8_SINT = 32,                             // DXGI_FORMAT_R8G8B8A8_SINT = 32,
    ALVR_FORMAT_R16G16_FLOAT = 34,                              // DXGI_FORMAT_R16G16_FLOAT = 34,
    ALVR_FORMAT_R16G16_UNORM = 35,                              // DXGI_FORMAT_R16G16_UNORM = 35,
    ALVR_FORMAT_R16G16_UINT = 36,                               // DXGI_FORMAT_R16G16_UINT = 36,
    ALVR_FORMAT_R16G16_SNORM = 37,                              // DXGI_FORMAT_R16G16_SNORM = 37,
    ALVR_FORMAT_R16G16_SINT = 38,                               // DXGI_FORMAT_R16G16_SINT = 38,
    ALVR_FORMAT_R32_FLOAT = 41,                                 // DXGI_FORMAT_R32_FLOAT = 41,
    ALVR_FORMAT_R32_UINT = 42,                                  // DXGI_FORMAT_R32_UINT = 42,
    ALVR_FORMAT_R32_SINT = 43,                                  // DXGI_FORMAT_R32_SINT = 43,
    ALVR_FORMAT_R8G8_UNORM = 49,                                // DXGI_FORMAT_R8G8_UNORM = 49,
    ALVR_FORMAT_R8G8_UINT = 50,                                 // DXGI_FORMAT_R8G8_UINT = 50,
    ALVR_FORMAT_R8G8_SNORM = 51,                                // DXGI_FORMAT_R8G8_SNORM = 51,
    ALVR_FORMAT_R8G8_SINT = 52,                                 // DXGI_FORMAT_R8G8_SINT = 52,
    ALVR_FORMAT_R16_FLOAT = 54,                                 // DXGI_FORMAT_R16_FLOAT = 54,
    ALVR_FORMAT_R16_UNORM = 56,                                 // DXGI_FORMAT_R16_UNORM = 56,
    ALVR_FORMAT_R16_UINT = 57,                                  // DXGI_FORMAT_R16_UINT = 57,
    ALVR_FORMAT_R16_SNORM = 58,                                 // DXGI_FORMAT_R16_SNORM = 58,
    ALVR_FORMAT_R16_SINT = 59,                                  // DXGI_FORMAT_R16_SINT = 59,
    ALVR_FORMAT_R8_UNORM = 61,                                  // DXGI_FORMAT_R8_UNORM = 61,
    ALVR_FORMAT_R8_UINT = 62,                                   // DXGI_FORMAT_R8_UINT = 62,
    ALVR_FORMAT_R8_SNORM = 63,                                  // DXGI_FORMAT_R8_SNORM = 63,
    ALVR_FORMAT_R8_SINT = 64,                                   // DXGI_FORMAT_R8_SINT = 64,
    ALVR_FORMAT_R9G9B9E5_SHAREDEXP = 67,                        // DXGI_FORMAT_R9G9B9E5_SHAREDEXP = 67,
    ALVR_FORMAT_BC1_UNORM = 71,                                 // DXGI_FORMAT_BC1_UNORM = 71,
    ALVR_FORMAT_BC1_UNORM_SRGB = 72,                            // DXGI_FORMAT_BC1_UNORM_SRGB = 72,
    ALVR_FORMAT_BC2_UNORM = 74,                                 // DXGI_FORMAT_BC2_UNORM = 74,
    ALVR_FORMAT_BC2_UNORM_SRGB = 75,                            // DXGI_FORMAT_BC2_UNORM_SRGB = 75,
    ALVR_FORMAT_BC3_UNORM = 77,                                 // DXGI_FORMAT_BC3_UNORM = 77,
    ALVR_FORMAT_BC3_UNORM_SRGB = 78,                            // DXGI_FORMAT_BC3_UNORM_SRGB = 78,
    ALVR_FORMAT_BC4_UNORM = 80,                                 // DXGI_FORMAT_BC4_UNORM = 80,
    ALVR_FORMAT_BC4_SNORM = 81,                                 // DXGI_FORMAT_BC4_SNORM = 81,
    ALVR_FORMAT_BC5_UNORM = 83,                                 // DXGI_FORMAT_BC5_UNORM = 83,
    ALVR_FORMAT_BC5_SNORM = 84,                                 // DXGI_FORMAT_BC5_SNORM = 84,
    ALVR_FORMAT_B5G6R5_UNORM = 85,                              // DXGI_FORMAT_B5G6R5_UNORM = 85,
    ALVR_FORMAT_B5G5R5A1_UNORM = 86,                            // DXGI_FORMAT_B5G5R5A1_UNORM = 86,
    ALVR_FORMAT_B8G8R8A8_UNORM = 87,                            // DXGI_FORMAT_B8G8R8A8_UNORM = 87,
    ALVR_FORMAT_B8G8R8X8_UNORM = 88,                            // DXGI_FORMAT_B8G8R8X8_UNORM = 88,
    ALVR_FORMAT_B8G8R8A8_UNORM_SRGB = 91,                       // DXGI_FORMAT_B8G8R8A8_UNORM_SRGB = 91,
    ALVR_FORMAT_B8G8R8X8_UNORM_SRGB = 93,                       // DXGI_FORMAT_B8G8R8X8_UNORM_SRGB = 93,
    ALVR_FORMAT_BC6H_UF16 = 95,                                 // DXGI_FORMAT_BC6H_UF16 = 95,
    ALVR_FORMAT_BC6H_SF16 = 96,                                 // DXGI_FORMAT_BC6H_SF16 = 96,
    ALVR_FORMAT_BC7_UNORM = 98,                                 // DXGI_FORMAT_BC7_UNORM = 98,
    ALVR_FORMAT_BC7_UNORM_SRGB = 99,                            // DXGI_FORMAT_BC7_UNORM_SRGB = 99,
    ALVR_FORMAT_FORCE_UINT = 0xffffffff                         // DXGI_FORMAT_FORCE_UINT = 0xffffffff
};


/**
***************************************************************************************************
* @brief base interface for all ALVR interfaces
***************************************************************************************************
*/

typedef IUnknown ALVRInterface;

/**
***************************************************************************************************
* @brief This interface is responsible to set / get optional properties
***************************************************************************************************
*/

enum ALVR_VARIANT_TYPE
{
    ALVR_VARIANT_EMPTY              = 0,

    ALVR_VARIANT_BOOL               = 1,
    ALVR_VARIANT_INT64              = 2,
    ALVR_VARIANT_DOUBLE             = 3,

    ALVR_VARIANT_RECT               = 4,
    ALVR_VARIANT_SIZE               = 5,
    ALVR_VARIANT_POINT              = 6,

    ALVR_VARIANT_STRING             = 7,  // value is char*
    ALVR_VARIANT_WSTRING            = 8,  // value is wchar*
    ALVR_VARIANT_INTERFACE          = 9,  // value is ALVRInterface*
};

struct ALVRVariantStruct
{
    ALVR_VARIANT_TYPE    type;
    union
    {
        bool            boolValue;
        int64_t         int64Value;
        double          doubleValue;
        char*           stringValue;
        wchar_t*        wstringValue;
        ALVRInterface*  pInterface;
        RECT            rectValue;
        SIZE            sizeValue;
        POINT           pointValue;
    };
};

ALVR_INTERFACE("A4A8B01D-2E99-447E-917A-67772781B10C")
ALVRPropertyStorage : public ALVRInterface
{
public:
    virtual ALVR_RESULT     ALVR_STD_CALL       SetProperty(const wchar_t* name, ALVRVariantStruct value) = 0;
    virtual ALVR_RESULT     ALVR_STD_CALL       GetProperty(const wchar_t* name, ALVRVariantStruct* pValue) const = 0;

    virtual bool            ALVR_STD_CALL       HasProperty(const wchar_t* name) const = 0;
    virtual size_t          ALVR_STD_CALL       GetPropertyCount() const = 0;
    virtual ALVR_RESULT     ALVR_STD_CALL       GetPropertyAt(size_t index, wchar_t* name, size_t nameSize, ALVRVariantStruct* pValue) const = 0;

    // to use these templates include LiquidVRVariant.h
    template<typename _T>
    ALVR_RESULT             ALVR_STD_CALL       SetProperty(const wchar_t* name, const _T& value);
    template<typename _T>
    ALVR_RESULT             ALVR_STD_CALL       GetProperty(const wchar_t* name, _T* pValue) const;
};
/**
***************************************************************************************************
* @brief This interface is responsible for syncronization between Gpus and GpuEngines
***************************************************************************************************
*/

ALVR_INTERFACE("F18348FB-BF86-4770-B5E4-B01C10B236BF")
ALVRGpuSemaphore : public ALVRInterface
{
};

/**
***************************************************************************************************
* @brief This interface is responsible for syncronization between Gpu and CPU
***************************************************************************************************
*/
ALVR_INTERFACE("4B15FFAE-0A80-4E6A-8478-1149390B8F6E")
ALVRFence : public ALVRInterface
{
    virtual ALVR_RESULT         ALVR_STD_CALL GetStatus(void) = 0;
    virtual ALVR_RESULT         ALVR_STD_CALL Wait(unsigned int timeout) = 0; // timeout in ms
};

/**
***************************************************************************************************
* @brief This interface is responsible for mapping timesamps between Gpu and CPU
***************************************************************************************************
*/
ALVR_INTERFACE("36CB702C-7741-48E3-B96A-1A40D844EE7D")
ALVRGpuTimeline : public ALVRInterface
{
    virtual ALVR_RESULT         ALVR_STD_CALL Recalibrate(void) = 0;
    virtual ALVR_RESULT         ALVR_STD_CALL GpuTimestampToQpc(uint64_t gpuTimestamp, uint64_t* pCpuPerfCounter) = 0;
};



/**
***************************************************************************************************
* @brief resource interface
***************************************************************************************************
*/

struct ALVRResourceD3D11
{
    ID3D11Resource* pResource;
};


ALVR_INTERFACE("CCB12C6D-046B-4515-8200-C0FE4BE4269B")
ALVRResource : public ALVRInterface
{
public:
    virtual ALVR_RESULT         ALVR_STD_CALL GetApiResource(ALVR_RENDER_API renderApi, void* pResource) = 0;
};


/**
***************************************************************************************************
* @brief constant buffer interface
***************************************************************************************************
*/
ALVR_INTERFACE("96CAAC8D-2712-4941-8ABA-A6C98172D748")
ALVRBuffer : public ALVRResource
{
public:
    virtual  size_t             ALVR_STD_CALL GetSize() const = 0;
    virtual  ALVR_RESULT        ALVR_STD_CALL Map(void **pData) = 0;
    virtual  ALVR_RESULT        ALVR_STD_CALL Unmap() = 0;
};

/**
***************************************************************************************************
* @brief surface interface
***************************************************************************************************
*/


ALVR_INTERFACE("BA9B18D4-1249-41A1-A97A-0BCBEFA52A80")
ALVRSurface : public ALVRResource
{
};



/**
***************************************************************************************************

/**
***************************************************************************************************
* @brief late latch constant buffer for use with D3D11 rendering
***************************************************************************************************
*/
ALVR_INTERFACE("752333C8-0CB0-4176-B1FD-B4C11D104A14")
ALVRLateLatchConstantBufferD3D11 : public ALVRInterface
{
    virtual ALVR_RESULT         ALVR_STD_CALL Update(void* pData, size_t offset, size_t size) = 0;
    virtual ALVR_RESULT         ALVR_STD_CALL QueueLatch(void) = 0;
    virtual ID3D11Buffer*       ALVR_STD_CALL GetIndexD3D11(void) = 0;
    virtual ID3D11Buffer*       ALVR_STD_CALL GetDataD3D11(void) = 0;
    virtual ALVRBuffer*         ALVR_STD_CALL GetIndex(void) = 0;
    virtual ALVRBuffer*         ALVR_STD_CALL GetData(void) = 0;
};

/**
***************************************************************************************************
* @brief This structure provides information about available GPUs
***************************************************************************************************
*/

struct ALVRGpuControlInfo
{
    unsigned int                    numGpus;                                    ///< Number of GPUs available for control
    unsigned int                    maskAllGpus;                                ///< GPU Mask representing all active GPUs
    unsigned int                    maskDisplayGpu;                            ///< GPU Mask representing the display GPU
};

/**
***************************************************************************************************
* @brief This interface is responsible for extending the ID3D11DeviceContext and adding
*   Liquid VR MGPU functionality.
***************************************************************************************************
*/
ALVR_INTERFACE("AB36ED9B-D3F6-493B-B049-406353063110")
ALVRMultiGpuDeviceContext : public ALVRPropertyStorage
{
    // GPU affinity management
    virtual ALVR_RESULT         ALVR_STD_CALL GetGpuControlInfo(ALVRGpuControlInfo* pInfo) = 0;
    virtual ALVR_RESULT         ALVR_STD_CALL SetGpuRenderAffinity(unsigned int affinityMask) = 0;

    // Resource control across GPUs
    virtual ALVR_RESULT         ALVR_STD_CALL MarkResourceAsInstanced(ID3D11Resource* pResource) = 0;
    virtual ALVR_RESULT         ALVR_STD_CALL TransferResource(
                                    ID3D11Resource*     pSrcResource,
                                    ID3D11Resource*     pDstResource,
                                    unsigned int        srcGpuIdx,
                                    unsigned int        dstGpuIdx,
                                    unsigned int        srcSubResourceIndex,
                                    unsigned int        dstSubResourceIndex,
                                    const D3D11_RECT*   pSrcRegion,
                                    const D3D11_RECT*   pDstRegion) = 0;

    virtual ALVR_RESULT         ALVR_STD_CALL TransferResourceEx(
                                    ID3D11Resource*     pSrcResource,
                                    ID3D11Resource*     pDstResource,
                                    unsigned int        srcGpuIdx,
                                    unsigned int        dstGpuIdx,
                                    unsigned int        srcSubResourceIndex,
                                    unsigned int        dstSubResourceIndex,
                                    const D3D11_RECT*   pSrcRegion,
                                    const D3D11_RECT*   pDstRegion,
                                    ALVR_GPU_ENGINE     transferEngine,
                                    bool                performSync) = 0;

};

/**
***************************************************************************************************
* @brief GPU affinity interface
***************************************************************************************************
*/
enum  ALVR_GPU_AFFINITY_FLAGS
{
    ALVR_GPU_AFFINITY_FLAGS_NONE = 0x00000000,
};

ALVR_INTERFACE("00573967-A082-4A88-9F9B-E49F41D13773")
ALVRGpuAffinity : public ALVRPropertyStorage
{
    virtual ALVR_RESULT         ALVR_STD_CALL EnableGpuAffinity(
                                    unsigned int flags) = 0;    //ALVR_GPU_AFFINITY_FLAGS
    virtual ALVR_RESULT         ALVR_STD_CALL DisableGpuAffinity() = 0;
    virtual ALVR_RESULT         ALVR_STD_CALL WrapDeviceD3D11(
                                    ID3D11Device* pDevice,
                                    ID3D11Device** ppWrappedDevice,
                                    ID3D11DeviceContext** ppWrappedContext,
                                    ALVRMultiGpuDeviceContext** ppMultiGpuDeviceContext) = 0;
};

/**
***************************************************************************************************
* @brief base interface for device extension
***************************************************************************************************
*/

ALVR_INTERFACE("09F7EE5E-0879-4C32-9922-0805A1CDDE85")
ALVRDeviceEx : public ALVRPropertyStorage
{
public:
};

/**
***************************************************************************************************
* @brief optional flags for late data latch buffer creation
***************************************************************************************************
*/

enum ALVR_LATE_LATCH_BUFFER_FLAGS
{
    ALVR_LATE_LATCH_NONE            = 0x00000000,
    ALVR_LATE_LATCH_SHARED_BUFFER   = 0x00000001,
};

/**
***************************************************************************************************
* @brief interface provides functionality for D3D11 device extension
***************************************************************************************************
*/

ALVR_INTERFACE("6E44501D-A923-46AF-8620-25D066B34879")
ALVRDeviceExD3D11 : public ALVRDeviceEx
{
public:
    // GPU sync
    virtual ALVR_RESULT         ALVR_STD_CALL CreateFence(ALVRFence** ppFence) = 0;
    virtual ALVR_RESULT         ALVR_STD_CALL SubmitFence(unsigned int gpuIdx, ALVRFence* pFence) = 0;

    virtual ALVR_RESULT         ALVR_STD_CALL CreateGpuSemaphore(ALVRGpuSemaphore** ppSemaphore) = 0;
    virtual ALVR_RESULT         ALVR_STD_CALL QueueSemaphoreSignal(ALVR_GPU_ENGINE gpuEngine, unsigned int gpuIdx, ALVRGpuSemaphore* pSemaphore) = 0;
    virtual ALVR_RESULT         ALVR_STD_CALL QueueSemaphoreWait(ALVR_GPU_ENGINE gpuEngine, unsigned int gpuIdx, ALVRGpuSemaphore* pSemaphore) = 0;

    virtual ALVR_RESULT         ALVR_STD_CALL CreateGpuTimeline(unsigned int gpuIdx, ALVRGpuTimeline** ppGpuTimeline) = 0;

    // late latch
    virtual ALVR_RESULT         ALVR_STD_CALL CreateLateLatchConstantBufferD3D11(
                                    size_t updateSize,
                                    unsigned int numberOfUpdates,
                                    unsigned int bufferFlags,                       //ALVR_LATE_LATCH_BUFFER_FLAGS
                                    ALVRLateLatchConstantBufferD3D11** ppBuffer) = 0;

};

enum ALVR_SURFACE_FLAGS
{
    ALVR_SURFACE_SHADER_INPUT = 0x00000001,
    ALVR_SURFACE_SHADER_OUTPUT = 0x00000002,
    ALVR_SURFACE_SHADER_RENDER_TARGET = 0x00000004,
};

enum ALVR_RESOURCE_API_SUPPORT
{
    ALVR_RESOURCE_API_ASYNC_COMPUTE = 1 << ALVR_RENDER_API_ASYNC_COMPUTE,
    ALVR_RESOURCE_API_D3D11 = 1 << ALVR_RENDER_API_D3D11,
};

enum ALVR_SURFACE_TYPE
{
    ALVR_SURFACE_1D = 0,
    ALVR_SURFACE_2D = 1,
    ALVR_SURFACE_3D = 2,
};

enum ALVR_BUFFER_FLAGS
{
    ALVR_BUFFER_SHADER_INPUT    = 0x00000001,
    ALVR_BUFFER_SHADER_OUTPUT   = 0x00000002,
    ALVR_BUFFER_CONSTANT        = 0x00000004,
};

enum ALVR_CPU_ACCESS_FLAGS
{
    ALVR_CPU_ACCESS_NONE        = 0x00000000,
    ALVR_CPU_ACCESS_READ        = 0x00000001,
    ALVR_CPU_ACCESS_WRITE       = 0x00000002,
};

enum ALVR_OPEN_SHARED_FLAGS
{
    ALVR_OPEN_SHARED_NONE       = 0x00000000,
    ALVR_OPEN_SHARED_NT_HANDLE  = 0x00000001,
};

struct ALVRSurfaceDesc
{
    ALVR_SURFACE_TYPE   type;
    ALVR_FORMAT         format;
    unsigned int        surfaceFlags;   // ALVR_SURFACE_FLAGS
    unsigned int        apiSupport;     // ALVR_RESOURCE_API_SUPPORT
    unsigned int        width;
    unsigned int        height;
    unsigned int        depth;
    ALVR_FORMAT         shaderInputFormat;  //optional, can be defined by format, put ALVR_FORMAT_UNKNOWN (0) if not needed
    ALVR_FORMAT         shaderOutputFormat; //optional, can be defined by format, put ALVR_FORMAT_UNKNOWN (0) if not needed
};

struct ALVRBufferDesc
{
    unsigned int        bufferFlags;        // ALVR_BUFFER_FLAGS
    unsigned int        cpuAccessFlags;     // ALVR_CPU_ACCESS_FLAGS
    unsigned int        apiSupport;         // ALVR_RESOURCE_API_SUPPORT
    size_t              size;
    size_t              structureStride;
    ALVR_FORMAT         format;             // typed buffer - put ALVR_FORMAT_UNKNOWN (0) if not needed
};

struct ALVROpenBufferDesc
{
    HANDLE              sharedHandle;
    unsigned int        bufferFlags;        // ALVR_BUFFER_FLAGS
    size_t              structureStride;
    ALVR_FORMAT         format;             // typed buffer - put ALVR_FORMAT_UNKNOWN  (0) if not needed
    unsigned int        openFlags;          // ALVR_OPEN_SHARED_FLAGS
};

struct ALVROpenSurfaceDesc
{
    HANDLE              sharedHandle;
    ALVR_FORMAT         format;
    unsigned int        surfaceFlags;       // ALVR_SURFACE_FLAGS
    unsigned int        openFlags;          // ALVR_OPEN_SHARED_FLAGS
    ALVR_FORMAT         shaderInputFormat;  //optional, can be defined by format, put ALVR_FORMAT_UNKNOWN (0) if not needed
    ALVR_FORMAT         shaderOutputFormat; //optional, can be defined by format, put ALVR_FORMAT_UNKNOWN (0) if not needed
};


enum ALVR_FILTER_MODE
{
    ALVR_FILTER_POINT = 0,
    ALVR_FILTER_LINEAR = 1,
};

enum ALVR_ADDRESS_MODE
{
    ALVR_ADDRESS_WRAP = 0,
    ALVR_ADDRESS_MIRROR = 1,
    ALVR_ADDRESS_CLAMP = 2,
};

struct ALVRSamplerDesc
{
    ALVR_FILTER_MODE    filterMode;
    ALVR_ADDRESS_MODE   addressU;
    ALVR_ADDRESS_MODE   addressV;
    ALVR_ADDRESS_MODE   addressW;
};

/**
***************************************************************************************************
* @brief Compute sampler
***************************************************************************************************
*/
ALVR_INTERFACE("F110CECF-D911-4A7B-8177-DD91F96D4595")
ALVRSampler : public ALVRInterface
{
public:
};
/**
***************************************************************************************************
* @brief Compute task
***************************************************************************************************
*/
ALVR_INTERFACE("0710A3DE-F367-453F-8DE3-89E19868837B")
ALVRComputeTask : public ALVRPropertyStorage
{
public:

    virtual  ALVR_RESULT        ALVR_STD_CALL BindConstantBuffer(unsigned int slot, ALVRBuffer *pBuffer) = 0;
    virtual  ALVR_RESULT        ALVR_STD_CALL BindSampler(unsigned int slot, ALVRSampler* pSampler) = 0;
    virtual  ALVR_RESULT        ALVR_STD_CALL BindInput(unsigned int slot, ALVRResource *pResource) = 0;
    virtual  ALVR_RESULT        ALVR_STD_CALL BindOutput(unsigned int slot, ALVRResource *pResource) = 0;
};

ALVR_INTERFACE("D030B60E-7BE1-4096-B81D-70A44B9714BB")
ALVRComputeTimestamp : public ALVRInterface
{
public:
    virtual  ALVR_RESULT        ALVR_STD_CALL GetValue(uint64_t* pValue) = 0;
};

enum ALVR_SHADER_MODEL
{
    ALVR_SHADER_MODEL_D3D11 = 0,
};

struct ALVRBox
{
    unsigned int left;
    unsigned int top;
    unsigned int front;
    unsigned int right;
    unsigned int bottom;
    unsigned int back;
};

struct ALVRPoint2D
{
    unsigned int x;
    unsigned int y;
};

struct ALVRPoint3D
{
    unsigned int x;
    unsigned int y;
    unsigned int z;
};

struct ALVRSize2D
{
    unsigned int width;
    unsigned int height;
};

struct ALVRSize3D
{
    unsigned int width;
    unsigned int height;
    unsigned int depth;
};

enum ALVR_COMPUTE_FLAGS
{
    ALVR_COMPUTE_NONE           = 0x00000000,
    ALVR_COMPUTE_HIGH_PRIORITY  = 0x00000001,
};

struct ALVRComputeContextDesc
{
    unsigned int flags; // ALVR_COMPUTE_FLAGS
};

ALVR_INTERFACE("3FC63136-68C2-42FE-B274-EEAEAD91C903")
ALVRComputeContext : public ALVRPropertyStorage
{
public:
    virtual  ALVR_RESULT        ALVR_STD_CALL CreateComputeTask(ALVR_SHADER_MODEL shaderModel, unsigned int flags, const void* pCode, size_t codeSize, ALVRComputeTask** ppTask) = 0;
    virtual  ALVR_RESULT        ALVR_STD_CALL CreateBuffer(const ALVRBufferDesc *pDesc, ALVRBuffer** ppBuffer) = 0;
    virtual  ALVR_RESULT        ALVR_STD_CALL CreateSampler(const ALVRSamplerDesc* pDesc, ALVRSampler** ppSampler) = 0;
    virtual  ALVR_RESULT        ALVR_STD_CALL CreateSurface(const ALVRSurfaceDesc* pDesc, ALVRSurface **ppSurface) = 0;
    virtual  ALVR_RESULT        ALVR_STD_CALL CreateTimestamp(ALVRComputeTimestamp** ppTimestamp) = 0;

    virtual  ALVR_RESULT        ALVR_STD_CALL Flush(ALVRFence* pFence) = 0;
    virtual  ALVR_RESULT        ALVR_STD_CALL QueueTask(ALVRComputeTask* pTask, const ALVRPoint3D* pOffset, const ALVRSize3D* pSize) = 0;
    virtual  ALVR_RESULT        ALVR_STD_CALL QueueTimestamp(ALVRComputeTimestamp* pTimestamp) = 0;
    virtual  ALVR_RESULT        ALVR_STD_CALL QueueSemaphoreSignal(ALVRGpuSemaphore* pSemaphore) = 0;
    virtual  ALVR_RESULT        ALVR_STD_CALL QueueSemaphoreWait(ALVRGpuSemaphore* pSemaphore) = 0;

    virtual  ALVR_RESULT        ALVR_STD_CALL QueueCopyBufferToBuffer(ALVRBuffer* pSrc, size_t srcOffset, ALVRBuffer* pDst, size_t dstOffset, size_t size) = 0;
    virtual  ALVR_RESULT        ALVR_STD_CALL QueueCopyBufferToSurface(ALVRBuffer* pSrc, size_t srcOffset, ALVRSurface* pDst, const ALVRBox *pDstBox) = 0;
    virtual  ALVR_RESULT        ALVR_STD_CALL QueueCopySurfaceToSurface(ALVRSurface* pSrc,const ALVRBox *pSrcBox, ALVRSurface* pDst, const ALVRPoint3D* pDstOffset) = 0;
    virtual  ALVR_RESULT        ALVR_STD_CALL QueueCopySurfaceToBuffer(ALVRSurface* pSrc, const ALVRBox *pSrcBox, ALVRBuffer* pDst, size_t dstOffset) = 0;

    virtual  ALVR_RESULT        ALVR_STD_CALL OpenSharedBuffer(const ALVROpenBufferDesc *pDesc, ALVRBuffer** ppBuffer) = 0;
    virtual  ALVR_RESULT        ALVR_STD_CALL OpenSharedSurface(const ALVROpenSurfaceDesc* pDesc, ALVRSurface **ppSurface) = 0;
};


/**
***************************************************************************************************
* @brief factory interface
***************************************************************************************************
*/

ALVR_INTERFACE("D6F6A5F6-D96B-4201-A08A-169618C90DB9")
ALVRFactory
{
    // MultiGPU
    virtual ALVR_RESULT         ALVR_STD_CALL CreateGpuAffinity(ALVRGpuAffinity** ppAffinity) = 0;

    // device extensions
    virtual ALVR_RESULT         ALVR_STD_CALL CreateALVRDeviceExD3D11(
                                    ID3D11Device* pd3dDevice,
                                    void* pConfigDesc, // optional configuration info reserved
                                    ALVRDeviceExD3D11** ppDevice) = 0;

    // compute
    virtual  ALVR_RESULT        ALVR_STD_CALL CreateComputeContext(
                                    ALVRDeviceEx* pDevice,
                                    unsigned int gpuIdx,
                                    ALVRComputeContextDesc* pDesc, // optional configuration info
                                    ALVRComputeContext** ppContext) = 0;

};

#if defined(_M_AMD64)
    #define ALVR_DLL_NAME    L"amdlvr64.dll"
#else
    #define ALVR_DLL_NAME    L"amdlvr32.dll"
#endif

#define ALVR_INIT_FUNCTION_NAME    "ALVRInit"

/**
***************************************************************************************************
* @brief entry point
***************************************************************************************************
*/
typedef ALVR_RESULT(ALVR_CDECL_CALL *ALVRInit_Fn)(uint64_t version, void **ppFactory);

/**
***************************************************************************************************
* end of file
***************************************************************************************************
*/

