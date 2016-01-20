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

//=================================================================================================================================
//
// Author: Jon Story
// AMD Corporation.
//
// These shaders are used for various basic utility rendering operations
//
//=================================================================================================================================
// $Id: $
//
// Last check-in: $DateTime: $
// Last edited by: $Author: Jon Story$
//=================================================================================================================================
// Copyright © AMD Corporation. All rights reserved.
//=================================================================================================================================


//=================================================================================================================================
// Constant buffer
//=================================================================================================================================

cbuffer cbUtility : register( b0 )
{
	float4x4 g_f4x4World;					// World matrix for object
	float4x4 g_f4x4View;					// View matrix
	float4x4 g_f4x4WorldViewProjection;		// World * View * Projection matrix
	float4 g_f4EyePt;						// Eye point	
	float2 g_f2RTSize;						// Render target
	float2 g_f2Padding;						// Padding 
    float4 g_f4LightDir;                    // Light direction vector
}

//=================================================================================================================================
// Buffers, Textures and Samplers
//=================================================================================================================================

// Textures
Texture2D g_txScene		: register( t0 );
Texture2D g_txHDAO		: register( t1 );
Texture2D g_txDiffuse	: register( t2 );
Texture2D g_txNormal	: register( t3 );

Texture2D g_txHDAO2		: register( t4 );
Texture2D g_txHDAO3		: register( t5 );

// Samplers
SamplerState        g_SamplePoint  : register( s0 );
SamplerState        g_SampleLinear : register( s1 );

// Lighting constants
//static float4 g_f4Directional1 = float4( 0.992, 1.0, 0.880, 0.0 );
//static float4 g_f4Directional2 = float4( 0.595, 0.6, 0.528, 0.0 );
//static float4 g_f4Ambient = float4(0.625, 0.674, 0.674, 0.0);

static float4 g_f4Directional1 = float4(0.992, 1.0, 0.880, 1.0);
static float4 g_f4Directional2 = float4(0.595, 0.6, 0.528, 1.0);
static float4 g_f4Ambient = float4(0.625, 0.674, 0.674, 1.0);

static float3 g_f3LightDir1 = float3( 1.705f, 5.557f, -9.380f );
static float3 g_f3LightDir2 = float3( -5.947f, -5.342f, -5.733f );


//=================================================================================================================================
// Vertex & Pixel shader structures
//=================================================================================================================================

struct VS_RenderSceneInput
{
    float3 f3Position	: POSITION;  
    float3 f3Normal		: NORMAL;     
    float2 f2TexCoord	: TEXCOORD0;
	float3 f3Tangent	: TANGENT;    
};

struct PS_RenderSceneInput
{
    float4 f4Position   : SV_Position;
	float2 f2TexCoord	: TEXCOORD0;
	float3 f3Normal     : NORMAL; 
	float3 f3Tangent    : TANGENT;
	float3 f3WorldPos	: TEXCOORD2;	
};

struct VS_RenderQuadInput
{
    float3 f3Position : POSITION; 
    float2 f2TexCoord : TEXCOORD0; 
};

struct PS_RenderQuadInput
{
    float4 f4Position : SV_Position; 
    float2 f2TexCoord : TEXCOORD0;
};

struct PS_RenderOutput
{
	float4 f4Color	    : SV_Target0;
//	float4 f4Normal	    : SV_Target1;
};


//=================================================================================================================================
// This shader computes standard transform and lighting
//=================================================================================================================================
PS_RenderSceneInput VS_RenderScene( VS_RenderSceneInput I )
{
    PS_RenderSceneInput O;
     
    // Transform the position from object space to homogeneous projection space
    O.f4Position = mul( float4( I.f3Position, 1.0f ), g_f4x4WorldViewProjection );
    
    // Transform the normal, tangent and position from object space to world space    
    O.f3WorldPos = mul( I.f3Position, (float3x3)g_f4x4World );
    O.f3Normal  = normalize( mul( I.f3Normal, (float3x3)g_f4x4World ) );
	O.f3Tangent = normalize( mul( I.f3Tangent, (float3x3)g_f4x4World ) );
    
	// Pass through tex coords
	O.f2TexCoord = I.f2TexCoord;
    
    return O;    
}


//=================================================================================================================================
// This shader outputs the pixel's color by passing through the lit 
// diffuse material color
//=================================================================================================================================
PS_RenderOutput PS_RenderScene( PS_RenderSceneInput I )
{
	PS_RenderOutput O;
	
    float3 LD1 = normalize(mul(g_f3LightDir1, (float3x3)g_f4x4World));
    float3 LD2 = normalize(mul(g_f3LightDir2, (float3x3)g_f4x4World));

    float4 f4Diffuse = g_txDiffuse.Sample(g_SampleLinear, I.f2TexCoord);
    float fSpecMask = f4Diffuse.a;
    float3 f3Norm = g_txNormal.Sample( g_SampleLinear, I.f2TexCoord ).xyz;
    f3Norm *= 2.0f;
    f3Norm -= float3( 1.0f, 1.0f, 1.0f );
    
    float3 f3Binorm = normalize( cross( I.f3Normal, I.f3Tangent ) );
    float3x3 f3x3BasisMatrix = float3x3( f3Binorm, I.f3Tangent, I.f3Normal );
    f3Norm = normalize( mul( f3Norm, f3x3BasisMatrix ) );
   
	// Write out the camera space normal 
//	O.f4Normal.x = f3Norm.x;	 
//	O.f4Normal.y = f3Norm.y;	
//	O.f4Normal.z = f3Norm.z;	
//	O.f4Normal.w = 0.0f;

    // Diffuse lighting
    float4 f4Lighting = saturate( dot( f3Norm, LD1.xyz ) ) * g_f4Directional1;
    f4Lighting += saturate( dot( f3Norm, LD2.xyz ) ) * g_f4Directional2;
    f4Lighting += ( g_f4Ambient );

    // Calculate specular power
    float3 f3ViewDir = normalize( g_f4EyePt.xyz - I.f3WorldPos );
    float3 f3HalfAngle = normalize( f3ViewDir + LD1.xyz );
    float4 f4SpecPower1 = pow( saturate( dot( f3HalfAngle, f3Norm ) ), 32 ) * g_f4Directional1;
    
    f3HalfAngle = normalize( f3ViewDir + LD2.xyz );
    float4 f4SpecPower2 = pow(saturate(dot(f3HalfAngle, f3Norm)), 32) * g_f4Directional2;


    O.f4Color = f4Lighting * f4Diffuse + ( f4SpecPower1 + f4SpecPower2 ) * fSpecMask;
     return O;
}


//=================================================================================================================================
// This shader outputs the pixel's color by passing through the lit 
// diffuse material color
//=================================================================================================================================
PS_RenderOutput PS_RenderSun( PS_RenderSceneInput I )
{
	PS_RenderOutput O;
    
    O.f4Color = float4( 1.0f, 0.0f, 0.0f, 1.0f );

    return O;
}


//=================================================================================================================================
// Simple shader for rendering screen quads
//=================================================================================================================================
PS_RenderQuadInput VS_RenderQuad( VS_RenderQuadInput I )
{
    PS_RenderQuadInput O;
    
    O.f4Position.x = I.f3Position.x;
	O.f4Position.y = I.f3Position.y;
    O.f4Position.z = I.f3Position.z;
    O.f4Position.w = 1.0f;
    
    O.f2TexCoord = I.f2TexCoord;
    
    return O;    
}


//=================================================================================================================================
// Render the scene 
//=================================================================================================================================
float4 PS_RenderSceneTexture( PS_RenderQuadInput I ) : SV_Target
{ 
	return g_txScene.Sample( g_SamplePoint, I.f2TexCoord );
}

//=================================================================================================================================
// EOF
//=================================================================================================================================









