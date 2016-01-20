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
#include "ImageShader2D.h"

class BarrelDistortionShader : public ImageShader2D
{
public:
    BarrelDistortionShader(float scaling);
    ~BarrelDistortionShader();

    virtual HRESULT Start();
    virtual HRESULT Stop();
    virtual HRESULT QueryEyes(UINT srcWidth, UINT srcHeight, UINT dstWidth, UINT dstHeight, ALVRSurface *pInputSurfaceL, ALVRSurface *pInputSurfaceR, ALVRSurface *pOutputSurfaceL, ALVRSurface *pOutputSurfaceR, bool bWait);

    virtual void    ResetSize();

    bool GetAntialiasing();
    void SetAntialiasing(bool bAntialiasing);

    void GetCoeffs(float *red, float *green, float *blue);
    void SetCoeffs(float red, float green, float blue);

protected:

#pragma pack(push)
#pragma pack(1)
    struct ParametersLR
    {   
        XMFLOAT3 k1Rgb;
        float zoomSrcToDst;
        
        XMFLOAT3 k2Rgb;
        UINT     enableAntialias;

        XMFLOAT4 backgrndColor;

        XMFLOAT2 origOutL;
        XMINT2 displayPosL;

        XMINT2 displaySizeL;
        XMINT2 inputPosL;         // LT in pixels of the input image picture (texture)

        XMINT2 inputSizeL;         // WxH size in pixels of the input image picture (texture)
        XMFLOAT2 origOutR;

        XMINT2 displayPosR;
        XMINT2 displaySizeR;

        XMINT2 inputPosR;         // LT in pixels of the input image picture (texture)
        XMINT2 inputSizeR;         // WxH size in pixels of the input image picture (texture)
    };
#pragma pack(pop)

    ALVRBuffer*	        m_pConstantBufferLR;
    ParametersLR                        m_ParamsLR;
    UINT                                m_Width;
    UINT                                m_Height;
    float                               m_Scaling;
    bool                                m_bAntialiasing;
};