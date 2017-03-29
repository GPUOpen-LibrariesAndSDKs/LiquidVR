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
// File: HUD.cpp
//
// Class definition for the AMD standard HUD interface.
//--------------------------------------------------------------------------------------


#include "../../DXUT/Core/DXUT.h"
#include "../../DXUT/Core/DXUTmisc.h"
#include "../../DXUT/Optional/DXUTgui.h"
#include "../../DXUT/Optional/SDKmisc.h"
#include "../../DXUT/Core/DDSTextureLoader.h"
#include "../inc/Sprite.h"
#include "../inc/HUD.h"

using namespace AMD;

//--------------------------------------------------------------------------------------
// Constructor
//--------------------------------------------------------------------------------------
HUD::HUD()
{
    m_pLogoSRV = NULL;
}


//--------------------------------------------------------------------------------------
// Destructor
//--------------------------------------------------------------------------------------
HUD::~HUD()
{
    SAFE_RELEASE( m_pLogoSRV );
}


//--------------------------------------------------------------------------------------
// Device creation hook function, that loads the AMD logo texture, and creates a sprite 
// object
//--------------------------------------------------------------------------------------
HRESULT HUD::OnCreateDevice( ID3D11Device* pd3dDevice )
{
    HRESULT hr;
    wchar_t str[MAX_PATH];

    m_Sprite.OnCreateDevice( pd3dDevice );
    
    V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, L"..\\AMD_SDK\\Media\\AMD.dds" ) );
    DirectX::CreateDDSTextureFromFile( pd3dDevice, str, nullptr, &m_pLogoSRV );
    DXUT_SetDebugName( m_pLogoSRV, "AMD.dds" );

    return hr;
}


//--------------------------------------------------------------------------------------
// Device destruction hook function, that releases the sprite object and 
//--------------------------------------------------------------------------------------
void HUD::OnDestroyDevice()
{
    m_Sprite.OnDestroyDevice();

    SAFE_RELEASE( m_pLogoSRV );
}


//--------------------------------------------------------------------------------------
// Resize swap chain hook function, that passes through to the sprite object 
//--------------------------------------------------------------------------------------
void HUD::OnResizedSwapChain( const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc )
{
    m_Sprite.OnResizedSwapChain( pBackBufferSurfaceDesc );
}


//--------------------------------------------------------------------------------------
// Render hook function, that calls the CDXUTDialog::OnRender method, and additionally 
// renders the AMD sprite
//--------------------------------------------------------------------------------------
void HUD::OnRender( float fElapsedTime )
{
    m_GUI.OnRender( fElapsedTime );
    m_Sprite.RenderSprite( m_pLogoSRV, DXUTGetDXGIBackBufferSurfaceDesc()->Width - 253, DXUTGetDXGIBackBufferSurfaceDesc()->Height, 256, 64, true, false );
}


Slider::Slider( CDXUTDialog& dialog, int id, int& y, const wchar_t* label, int min, int max, int& value ) :
	m_Value( value ),
	m_szLabel( label )
{
	dialog.AddStatic( id + 1000000, L"", AMD::HUD::iElementOffset, y += AMD::HUD::iElementDelta, AMD::HUD::iElementWidth, AMD::HUD::iElementHeight, false, &m_pLabel );
	dialog.AddSlider( id, AMD::HUD::iElementOffset, y += AMD::HUD::iElementDelta, AMD::HUD::iElementWidth, AMD::HUD::iElementHeight, min, max, m_Value, false, &m_pSlider );

	dialog.AddControl( this );

	OnGuiEvent();
}


void Slider::OnGuiEvent()
{
	m_Value = m_pSlider->GetValue();

	wchar_t buff[ 1024 ];
	swprintf_s( buff, 1024, L"%s: %d", m_szLabel, m_Value );

	m_pLabel->SetText( buff );
}


void Slider::SetEnabled( bool enable )
{
	m_pLabel->SetEnabled( enable );
	m_pSlider->SetEnabled( enable );
}


void Slider::SetVisible( bool visible )
{
	m_pLabel->SetVisible( visible );
	m_pSlider->SetVisible( visible );
}


void Slider::SetValue( int value )
{
	m_pSlider->SetValue( value );
	OnGuiEvent();
}

//--------------------------------------------------------------------------------------
// EOF
//--------------------------------------------------------------------------------------