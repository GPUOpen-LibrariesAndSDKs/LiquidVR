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

#include "..\inc\MouseInput.h"
//#include "..\..\common\inc\ApplicationContext.h"
#include "..\..\DXUT\Core\DXUT.h"
#include <DirectXMath.h>
using namespace DirectX;

static void increase_timer_precision();
double get_high_precision_clock(); // in sec

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
m_EmulateAngle(0.0f)
{
	increase_timer_precision(); // increases accuracy of Sleep() to 2 ms (orignal  - 15 ms)
}
//-------------------------------------------------------------------------------------------------
MouseInput::~MouseInput()
{
	Terminate();
}
//-------------------------------------------------------------------------------------------------
HRESULT MouseInput::Init(HINSTANCE hinst, HWND hWnd, MouseInputCallback *pCallback, UINT mouseEventsPerSecond)
{
	Terminate();
	m_hWnd = hWnd;
	m_hInstance = hinst;
	m_pCallback = pCallback;
	m_uiMouseEventsPerSecond = mouseEventsPerSecond;
	m_hWatchThread = CreateThread(NULL, 0, ThreadProc, this, 0, NULL);
	if (m_hWatchThread == NULL)
	{
		return E_FAIL;
	}

	return S_OK;
}
//-------------------------------------------------------------------------------------------------
HRESULT MouseInput::InitDirectInput()
{
	HRESULT hr = S_OK;
	V_RETURN(DirectInput8Create(m_hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8W, (void**)&m_pDirectInput8W, NULL));
	
//	V_RETURN(CoCreateInstance(CLSID_DirectInput8, NULL, CLSCTX_INPROC_SERVER, IID_IDirectInput8W, (void**)&m_pDirectInput8W));
//	m_pDirectInput8W->Initialize(hinst, DIRECTINPUT_VERSION);

	V_RETURN(m_pDirectInput8W->CreateDevice(GUID_SysMouse, &m_pMouseDevice, NULL));


	V_RETURN(m_pMouseDevice->SetCooperativeLevel(m_hWnd, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND));
	V_RETURN(m_pMouseDevice->SetDataFormat(&c_dfDIMouse));

	m_bMouseAcquired = m_pMouseDevice->Acquire() == DI_OK;

	return S_OK;
}
//-------------------------------------------------------------------------------------------------
HRESULT MouseInput::Terminate()
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
	SAFE_RELEASE(m_pMouseDevice);
	SAFE_RELEASE(m_pDirectInput8W);
	m_bMouseAcquired = false;
	m_pCallback = NULL;
	m_hWnd = NULL;
	m_hInstance = NULL;
	m_uiMouseEventsPerSecond = 100;
	return S_OK;
}
//-------------------------------------------------------------------------------------------------
HRESULT MouseInput::TryMouseUpdate(bool &bLeftDown)
{
	bLeftDown = false;
	HRESULT         hr = S_OK;
	if (m_pMouseDevice == NULL)
	{
		return hr;
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
		hr = S_FALSE;
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

		if (m_pCallback != NULL)
		{
			m_pCallback->MouseEvent(m_LastMousePos.x, m_LastMousePos.y, bLeftDown, bRightDown);
			hr = S_OK;
		}
	}
	return hr;
}
//-------------------------------------------------------------------------------------------------
HRESULT MouseInput::TryMouseEmulate(bool &bLeftDown)
{


	RECT client;
	GetClientRect(m_hWnd, &client);

	ClientToScreen(m_hWnd, (POINT*)&client);
	ClientToScreen(m_hWnd, ((POINT*)&client)+1);

	long centerX = (client.right + client.left) / 2 - (client.right - client.left) / 4;
	long centerY = (client.bottom + client.top) / 2;
	/*
	float radius = min((client.right - client.left) / 2, client.bottom - client.top) / 2.0f / 1.5f;


	long x = centerX + (long)(radius * cos(m_EmulateAngle));
	long y = centerY + (long)(radius * sin(m_EmulateAngle));

	m_EmulateAngle += XM_2PI / 360.0f; // 10 degrees each iteration
	if (m_EmulateAngle >= XM_2PI)
	{
		m_EmulateAngle = 0;
	}
	*/
	m_EmulateAngle += 2;
	if (m_EmulateAngle > client.bottom)
	{
		m_EmulateAngle = 0;
	}
	long x = centerX;
	long y = (long)m_EmulateAngle;

	bLeftDown = true;
	if (m_pCallback != NULL)
	{
		m_pCallback->MouseEvent(x, y, bLeftDown, false);
	}
	return S_OK;
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

	double dStartTime = get_high_precision_clock(); // in sec

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

		double dCurrentTime = get_high_precision_clock(); // in sec
		if (dCurrentTime - dStartTime >= 1.0) // second passed
		{
//			DXUTOutputDebugStringW(L"+++ Mouse Events in one second = %d\n", iEventCountInOneSec);
			iEventCountInOneSec = 0;
			dStartTime = dCurrentTime;
		}
		if (hr == S_OK && bLeftDown) // event sent
		{
			iEventCountInOneSec++;
		}
		if (FAILED(hr))
		{
			int a = 1;
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
		if (sleepTime > 2000)
		{
			int a = 1;
		}
		Sleep(sleepTime);

	}
	return 0;
}
//-------------------------------------------------------------------------------------------------
// increases accuracy of Sleep() to 2 ms (orignal  - 15 ms)
//-------------------------------------------------------------------------------------------------
static void increase_timer_precision()
{
#if !defined(METRO_APP)
	typedef NTSTATUS(CALLBACK * NTSETTIMERRESOLUTION)(IN ULONG DesiredTime, IN BOOLEAN SetResolution, OUT PULONG ActualTime);
	typedef NTSTATUS(CALLBACK * NTQUERYTIMERRESOLUTION)(OUT PULONG MaximumTime, OUT PULONG MinimumTime, OUT PULONG CurrentTime);

	HINSTANCE hNtDll = LoadLibrary(L"NTDLL.dll");
	if (hNtDll != NULL)
	{
		ULONG MinimumResolution = 0;
		ULONG MaximumResolution = 0;
		ULONG ActualResolution = 0;

		NTQUERYTIMERRESOLUTION NtQueryTimerResolution = (NTQUERYTIMERRESOLUTION)GetProcAddress(hNtDll, "NtQueryTimerResolution");
		NTSETTIMERRESOLUTION NtSetTimerResolution = (NTSETTIMERRESOLUTION)GetProcAddress(hNtDll, "NtSetTimerResolution");

		if (NtQueryTimerResolution != NULL && NtSetTimerResolution != NULL)
		{
			NtQueryTimerResolution(&MinimumResolution, &MaximumResolution, &ActualResolution);
			if (MaximumResolution != 0)
			{
				NtSetTimerResolution(MaximumResolution, TRUE, &ActualResolution);
				NtQueryTimerResolution(&MinimumResolution, &MaximumResolution, &ActualResolution);

				// if call NtQueryTimerResolution() again it will return the same values but precision is actually increased
			}
		}
		FreeLibrary(hNtDll);
	}
#endif
}
double get_high_precision_clock() // in sec
{
	static int state = 0;
	static LARGE_INTEGER Frequency;
	static LARGE_INTEGER StartCount;
	if (state == 0)
	{
		if (QueryPerformanceFrequency(&Frequency))
		{
			state = 1;
			QueryPerformanceCounter(&StartCount);
		}
		else
		{
			state = 2;
		}
	}
	if (state == 1)
	{
		LARGE_INTEGER PerformanceCount;
		if (QueryPerformanceCounter(&PerformanceCount))
		{
			return static_cast<double>(PerformanceCount.QuadPart - StartCount.QuadPart) / static_cast<double>(Frequency.QuadPart);
		}
	}
	return static_cast<double>(GetTickCount()) * 1000;
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
