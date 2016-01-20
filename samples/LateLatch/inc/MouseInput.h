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

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>


class MouseInputCallback
{
public:
	virtual void MouseEvent(LONG posX, LONG posY, bool bLeftDown, bool bRightDown) = 0;
};

class MouseInput
{
public:
	MouseInput();
	~MouseInput();

	HRESULT Init(HINSTANCE hinst, HWND hWnd, MouseInputCallback *pCallback, UINT mouseEventsPerSecond);
	HRESULT Terminate();
	void	SetEmulation(bool emulation){ m_bEmulation = emulation; }
	bool	GetEmulation(){ return m_bEmulation; }
protected:
	HRESULT InitDirectInput();
	HRESULT TryMouseUpdate(bool &bLeftDown);
	HRESULT TryMouseEmulate(bool &bLeftDown);
	static DWORD WINAPI ThreadProc(_In_  LPVOID lpParameter);
	DWORD ThreadRun();

	HWND				m_hWnd;
	HINSTANCE			m_hInstance;
	IDirectInput8W		*m_pDirectInput8W;
	IDirectInputDevice8	*m_pMouseDevice;
	bool				m_bMouseAcquired;
	HANDLE				m_hWatchThread;
	volatile LONG		m_lThreadStop;
	MouseInputCallback *m_pCallback;
	POINT				m_LastMousePos;
	UINT				m_uiMouseEventsPerSecond;
	bool				m_bEmulation;
	float				m_EmulateAngle;
};