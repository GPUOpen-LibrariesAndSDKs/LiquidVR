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

#include "MouseInput.h"

//-------------------------------------------------------------------------------------------------
MouseInput::MouseInput() :
m_pDirectInput8W(NULL),
m_pMouseDevice(NULL),
m_bMouseAcquired(false),
m_hWatchThread(NULL),
m_lThreadStop(0),
m_pCallback(NULL),
m_hWnd(NULL),
m_hInstance(NULL),
m_uiMouseEventsPerSecond(100),
m_bEmulation(true),
m_EmulatePosition(0.0f)
{
	increase_timer_precision(); // increases accuracy of Sleep() to 2 ms (orignal  - 15 ms)
}
//-------------------------------------------------------------------------------------------------
MouseInput::~MouseInput()
{
	Terminate();
}
//-------------------------------------------------------------------------------------------------
ALVR_RESULT  MouseInput::Init(HINSTANCE hinst, HWND hWnd, MouseInputCallback *pCallback, UINT mouseEventsPerSecond)
{
	Terminate();
	m_hWnd = hWnd;
	m_hInstance = hinst;
	m_pCallback = pCallback;
	m_uiMouseEventsPerSecond = mouseEventsPerSecond;
	m_hWatchThread = CreateThread(NULL, 0, ThreadProc, this, 0, NULL);

    CHECK_RETURN(m_hWatchThread != NULL, ALVR_FAIL, L"CreateThread(0 failed");
	return ALVR_OK;
}
//-------------------------------------------------------------------------------------------------
ALVR_RESULT  MouseInput::InitDirectInput()
{
	HRESULT hr = S_OK;
	hr = DirectInput8Create(m_hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8W, (void**)&m_pDirectInput8W, NULL);
    CHECK_HRESULT_ERROR_RETURN(hr, L"DirectInput8Create() failed");

	hr = m_pDirectInput8W->CreateDevice(GUID_SysMouse, &m_pMouseDevice, NULL);
    CHECK_HRESULT_ERROR_RETURN(hr, L"m_pDirectInput8W->CreateDevice() failed");


    hr = m_pMouseDevice->SetCooperativeLevel(m_hWnd, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
    CHECK_HRESULT_ERROR_RETURN(hr, L"SetCooperativeLevel() failed");
    hr = m_pMouseDevice->SetDataFormat(&c_dfDIMouse);
    CHECK_HRESULT_ERROR_RETURN(hr, L"SetDataFormat() failed");

	m_bMouseAcquired = m_pMouseDevice->Acquire() == DI_OK;

	return ALVR_OK;
}
//-------------------------------------------------------------------------------------------------
ALVR_RESULT MouseInput::Terminate()
{
	// stop thread
	if (m_hWatchThread != NULL)
	{
		// tell thread to stop
		m_lThreadStop = 1;
		// wait 
		while (true)
		{
			DWORD ExitCode = 0;
			if (!GetExitCodeThread(m_hWatchThread, &ExitCode) || ExitCode != STILL_ACTIVE)
			{
				break;
			}
			Sleep(2);
		}
		m_lThreadStop = 0;
	}
    m_pMouseDevice.Release();
	m_pDirectInput8W.Release();
	m_bMouseAcquired = false;
	m_pCallback = NULL;
	m_hWnd = NULL;
	m_hInstance = NULL;
	m_uiMouseEventsPerSecond = 100;
	return ALVR_OK;
}
//-------------------------------------------------------------------------------------------------
ALVR_RESULT MouseInput::TryMouseUpdate(bool &bLeftDown)
{
	bLeftDown = false;
    ALVR_RESULT res = ALVR_OK;
    HRESULT     hr = S_OK;
    if(m_pMouseDevice == NULL)
	{
		return res;
	}
	m_pMouseDevice->Poll();

	DIMOUSESTATE		mouseState;

	for (int i = 0; i < 3; i++) // try 3 times
	{
		if ((hr = m_pMouseDevice->GetDeviceState(sizeof(mouseState), &mouseState)) != DI_OK)
		{
			m_pMouseDevice->Unacquire();
			m_bMouseAcquired = m_pMouseDevice->Acquire() == DI_OK;
		}
		else
		{
			break;
		}

	}
	if (hr == S_OK)
	{
		res = ALVR_FALSE;
		// update mouse pos here
		LONG posX = mouseState.lX;
		LONG posY = mouseState.lY;
		bLeftDown = (mouseState.rgbButtons[0] & 0x80) != 0;
		bool bRightDown = (mouseState.rgbButtons[1] & 0x80) != 0;
		if (posX == 0 && posY == 0 )
		{
			GetCursorPos(&m_LastMousePos);
		}
		else
		{
			m_LastMousePos.x += posX;
			m_LastMousePos.y += posY;
		}

        RECT client;
        GetClientRect(m_hWnd, &client);

        POINT leftTop;
        leftTop.x = client.left;
        leftTop.y = client.top;
        ClientToScreen(m_hWnd, &leftTop);

        POINT rightBottom;
        rightBottom.x = client.right;
        rightBottom.y = client.bottom;
        ClientToScreen(m_hWnd, &rightBottom);

        long centerX = (rightBottom.x + leftTop.x) / 2 - (rightBottom.x - leftTop.x) / 4;
		if (m_pCallback != NULL)
		{
            m_pCallback->MouseEvent(centerX, m_LastMousePos.y, bLeftDown, bRightDown);
            res = ALVR_OK;
		}
	}
	return res;
}
//-------------------------------------------------------------------------------------------------
ALVR_RESULT MouseInput::TryMouseEmulate(bool &bLeftDown)
{
	RECT client;
	GetClientRect(m_hWnd, &client);

    POINT leftTop;
    leftTop.x = client.left;
    leftTop.y = client.top;
    ClientToScreen(m_hWnd, &leftTop);

    POINT rightBottom;
    rightBottom.x = client.right;
    rightBottom.y = client.bottom;
    ClientToScreen(m_hWnd, &rightBottom);

    long centerX = (rightBottom.x + leftTop.x) / 2 - (rightBottom.x - leftTop.x) / 4;
    long centerY = (rightBottom.y + leftTop.y) / 2;

    m_EmulatePosition += 2;
    if (m_EmulatePosition > rightBottom.y - leftTop.y)
	{
        m_EmulatePosition = 0;
	}
	long x = centerX;
    long y = (long)m_EmulatePosition + leftTop.y;

	bLeftDown = true;
	if (m_pCallback != NULL)
	{
		m_pCallback->MouseEvent(x, y, bLeftDown, false);
	}
	return ALVR_OK;
}
//-------------------------------------------------------------------------------------------------
DWORD WINAPI MouseInput::ThreadProc(_In_  LPVOID lpParameter)
{
	return ((MouseInput*)lpParameter)->ThreadRun();
}
//-------------------------------------------------------------------------------------------------
DWORD MouseInput::ThreadRun()
{
	InitDirectInput();

	double dStartTime = high_precision_clock() / 10000000.; // in sec

	int iEventCountInOneSec = 0;


	while (!m_lThreadStop)
	{
		HRESULT hr = S_OK;
		bool bLeftDown = false;
		if (m_bEmulation)
		{
			hr = TryMouseEmulate(bLeftDown);
		}
		else
		{
			hr = TryMouseUpdate(bLeftDown);
		}

        double dCurrentTime = high_precision_clock() / 10000000.; // in sec
		if (dCurrentTime - dStartTime >= 1.0) // second passed
		{
			iEventCountInOneSec = 0;
			dStartTime = dCurrentTime;
		}
		if (hr == S_OK && bLeftDown) // event sent
		{
			iEventCountInOneSec++;
		}

		UINT uiEventsLeftInSecond = m_uiMouseEventsPerSecond - iEventCountInOneSec;
		if (uiEventsLeftInSecond == 0)
		{
			uiEventsLeftInSecond = 1;
		}
		double dTimeLefInSecond = 1.0 - (dCurrentTime - dStartTime);
		DWORD sleepTime = (DWORD)(dTimeLefInSecond * 1000 / uiEventsLeftInSecond);
		if (sleepTime == 0)
		{
			sleepTime = 1;
		}
		Sleep(sleepTime);
	}
	return 0;
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
