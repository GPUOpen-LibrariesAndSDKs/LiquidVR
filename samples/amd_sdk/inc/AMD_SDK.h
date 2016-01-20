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
// File: AMD_SDK.h
//
// Library include file, to drag in all AMD SDK helper classes and functions.
//--------------------------------------------------------------------------------------
#ifndef __AMD_SDK_H__
#define __AMD_SDK_H__


#define VENDOR_ID_AMD		(0x1002)
#define VENDOR_ID_NVIDIA	(0x10DE)
#define VENDOR_ID_INTEL		(0x8086)


#include "..\\..\\DXUT\\Core\\DXUT.h"
#include "..\\..\\DXUT\\Core\\DXUTmisc.h"
#include "..\\..\\DXUT\\Optional\\DXUTgui.h"
#include "..\\..\\DXUT\\Optional\\SDKmisc.h"
#include "..\\..\\DXUT\\Optional\\SDKMesh.h"


// AMD helper classes and functions
#include "Timer.h"
#include "ShaderCache.h"
#include "HelperFunctions.h"
#include "Sprite.h" 
#include "Magnify.h"
#include "MagnifyTool.h"
#include "HUD.h"
#include "Geometry.h"
#include "LineRender.h"


// Profile helpers for timing and marking up as D3D perf blocks
#define AMD_PROFILE_RED		D3DCOLOR_XRGB( 255, 0, 0 )
#define AMD_PROFILE_GREEN	D3DCOLOR_XRGB( 0, 255, 0 )
#define AMD_PROFILE_BLUE	D3DCOLOR_XRGB( 0, 0, 255 )


#define AMDProfileBegin( col, name ) DXUT_BeginPerfEvent( col, name ); TIMER_Begin( col, name )
#define AMDProfileEnd() TIMER_End() DXUT_EndPerfEvent();

struct AMDProfileEventClass
{
	AMDProfileEventClass( unsigned int col, LPCWSTR name ) { AMDProfileBegin( col, name ); }
	~AMDProfileEventClass() { AMDProfileEnd() }
};

#define AMDProfileEvent( col, name ) AMDProfileEventClass __amd_profile_event( col, name )



#endif

//--------------------------------------------------------------------------------------
// EOF
//--------------------------------------------------------------------------------------

