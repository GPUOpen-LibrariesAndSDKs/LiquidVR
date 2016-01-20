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

//--------------------------------------------------------------------------------------
// Globals
//--------------------------------------------------------------------------------------
cbuffer cbPerObject : register( b0 )
{
//	matrix		g_mViewProjection		: packoffset( c0 );
//	matrix		g_mWorld				: packoffset( c4 );
	matrix		g_mViewProjection		;
	matrix		g_mWorld				;
};


cbuffer cbObjectPosition : register(b1)
{
	matrix		g_mPosition	: packoffset(c0);
};

cbuffer cbStatic : register(b2)
{
	float3		g_vLightDir				: packoffset(c0);
	float		g_fAmbient : packoffset(c0.w);
};

//--------------------------------------------------------------------------------------
// Input / Output structures
//--------------------------------------------------------------------------------------
struct VS_INPUT
{
	float3 Pos          : POSITION;         //position
	float3 Norm         : NORMAL;           //normal
	float2 Tex          : TEXCOORD0;        //texture coordinate
};

struct VS_OUTPUT
{
	float4 Pos : SV_POSITION;
	float4 Diffuse : COLOR0;
	float2 Tex : TEXCOORD1;
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
VS_OUTPUT VSMain( VS_INPUT Input )
{
	/*
	VS_OUTPUT Output;
	
	Output.vPosition = mul(Input.vPosition, mul(mul(g_mWorld, g_mPosition), g_mWorldViewProjection));
	Output.vNormal = mul(Input.vNormal, (float3x3)mul(g_mWorld, g_mPosition ));
	Output.vTexcoord = Input.vTexcoord;
	
	return Output;
	*/
//	matrix		world = g_mWorld;
	matrix		world = mul(g_mWorld, g_mPosition);
	matrix		worldViewProjection = mul(world, g_mViewProjection);
	VS_OUTPUT output = (VS_OUTPUT)0;

	float Puffiness = 0;
	Input.Pos += Input.Norm * Puffiness;

	output.Pos = mul(float4(Input.Pos, 1), worldViewProjection);
	float3 vNormalWorldSpace = normalize(mul(Input.Norm, (float3x3)world));

	float fLighting = saturate(dot(vNormalWorldSpace, g_vLightDir));
	fLighting = max(fLighting, g_fAmbient);
	output.Diffuse.rgb = fLighting;
	output.Diffuse.a = 1.0f;

	output.Tex = Input.Tex;

	return output;

}

