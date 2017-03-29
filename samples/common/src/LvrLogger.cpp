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

#include "../inc/LvrLogger.h"
#include <iostream>
#include <iomanip>

void ChangeTextColor(LogType type)
{
#if !defined(METRO_APP)
    HANDLE hCmd = GetStdHandle(STD_OUTPUT_HANDLE);
    static int First = 0;
    static WORD wDefaultAttributes = 0;
    if(!First)
    {
        First = 1;
        CONSOLE_SCREEN_BUFFER_INFO info;
        GetConsoleScreenBufferInfo(hCmd, &info);
        wDefaultAttributes = info.wAttributes;
    }


    switch(type)
    {
    case LogTypeInfo:
        SetConsoleTextAttribute(hCmd, FOREGROUND_INTENSITY);
        break;
    case LogTypeSuccess:
        SetConsoleTextAttribute(hCmd, FOREGROUND_GREEN);
        break;
    case LogTypeError:
        SetConsoleTextAttribute(hCmd, FOREGROUND_RED);
        break;
    case LogTypeDefault:
        SetConsoleTextAttribute(hCmd, wDefaultAttributes);
        break;
    }
#endif
}


void WriteLog(const wchar_t* message, LogType type)
{

    ChangeTextColor(type);
    wprintf(message);
    ChangeTextColor(LogTypeDefault);
}

//------------------------------------------------------------------------------------------------------
static const wchar_t* resultString[] =
{
    L"OK",                        //ALVR_OK = 0,
    L"False",                    //ALVR_FALSE = 1,
    L"Fail",                    //ALVR_FAIL = 2,
    L"Invalid argument",        //ALVR_INVALID_ARGUMENT = 3,
    L"Not initialized",            //ALVR_NOT_INITIALIZED = 4,
    L"Not sufficient buffer",    //ALVR_NOT_SUFFICIENT_BUFFER = 5,
    L"Not implemented",            //ALVR_NOT_IMPLEMENTED = 6,
    L"NULL pointer",            //ALVR_NULL_POINTER = 7,
    L"Already initialized",        //ALVR_ALREADY_INITIALIZED = 8,
    L"Unsupported version",        //ALVR_UNSUPPORTED_VERSION = 9,
    L"Out of memory",            // ALVR_OUT_OF_MEMORY = 10,
    L"Display removed",        //ALVR_DISPLAY_REMOVED = 11,
    L"Display used",        //ALVR_DISPLAY_USED = 12,
    L"Display unavailable",        //ALVR_DISPLAY_UNAVAILABLE = 13,
    L"Display not enabled",        //ALVR_DISPLAY_NOT_ENABLED = 14,
    L"Outstanding present frame",        //ALVR_OUTSTANDING_PRESENT_FRAME = 15,
    L"Device lost",        //ALVR_DEVICE_LOST = 16,
    L"Unavailable",             //ALVR_UNAVAILABLE = 17,
    L"Not ready",               //ALVR_NOT_READY = 18,
    L"Timeout",                 //ALVR_TIMEOUT = 19,
    L"Resource is not bound",   // ALVR_RESOURCE_IS_NOT_BOUND
    L"Unsupported",             //ALVR_UNSUPPORTED
    L"Incompatible driver", // ALVR_INCOMPATIBLE_DRIVER
    L"Device mismatch", // ALVR_DEVICE_MISMATCH
    L"Invalid display timing", //ALVR_INVALID_DISPLAY_TIMING     = 24,
    L"Invalid display resolution", //ALVR_INVALID_DISPLAY_RESOLUTION = 25,
    L"Invalid display scaling", //ALVR_INVALID_DISPLAY_SCALING    = 26,
    L"Invalid display out of spec", //ALVR_INVALID_DISPLAY_OUT_OF_SPEC = 27,
};

const wchar_t* ALVRResultToString(ALVR_RESULT res)
{
    if((size_t)res < _countof(resultString))
    {
        return resultString[(size_t)res];
    }
    static wchar_t text[100];
    swprintf(text, _countof(text), L"Unknown Error (%d)", (int)res);
    return text;
}
//-------------------------------------------------------------------------------------------------
__int64 high_precision_clock()
{
    static int state = 0;
    static LARGE_INTEGER Frequency;
    static LARGE_INTEGER StartCount;
    if(state == 0)
    {
        if(QueryPerformanceFrequency(&Frequency))
        {
            state = 1;
            QueryPerformanceCounter(&StartCount);
        }
        else
        {
            state = 2;
        }
    }
    if(state == 1)
    {
        LARGE_INTEGER PerformanceCount;
        if(QueryPerformanceCounter(&PerformanceCount))
        {
            return static_cast<__int64>((PerformanceCount.QuadPart - StartCount.QuadPart) * 10000000LL / Frequency.QuadPart);
        }
    }
#if defined(METRO_APP)
    return GetTickCount64() * 10;

#else
    return GetTickCount() * 10;
#endif
}
//-------------------------------------------------------------------------------------------------
__int64 qpc_to_high_precision_clock(__int64 qpc)
{
    static int state = 0;
    static LARGE_INTEGER Frequency;
    static LARGE_INTEGER StartCount;
    if(state == 0)
    {
        if(QueryPerformanceFrequency(&Frequency))
        {
            state = 1;
            QueryPerformanceCounter(&StartCount);
        }
        else
        {
            state = 2;
        }
    }
    if(state == 1)
    {
        return static_cast<__int64>((qpc - StartCount.QuadPart) * 10000000LL / Frequency.QuadPart);
    }
#if defined(METRO_APP)
    return GetTickCount64() * 10;

#else
    return GetTickCount() * 10;
#endif
}
//-------------------------------------------------------------------------------------------------
void increase_timer_precision()
{
#if !defined(METRO_APP)
    typedef NTSTATUS(CALLBACK * NTSETTIMERRESOLUTION)(IN ULONG DesiredTime, IN BOOLEAN SetResolution, OUT PULONG ActualTime);
    typedef NTSTATUS(CALLBACK * NTQUERYTIMERRESOLUTION)(OUT PULONG MaximumTime, OUT PULONG MinimumTime, OUT PULONG CurrentTime);

    HINSTANCE hNtDll = LoadLibrary(L"NTDLL.dll");
    if(hNtDll != NULL)
    {
        ULONG MinimumResolution = 0;
        ULONG MaximumResolution = 0;
        ULONG ActualResolution = 0;

        NTQUERYTIMERRESOLUTION NtQueryTimerResolution = (NTQUERYTIMERRESOLUTION)GetProcAddress(hNtDll, "NtQueryTimerResolution");
        NTSETTIMERRESOLUTION NtSetTimerResolution = (NTSETTIMERRESOLUTION)GetProcAddress(hNtDll, "NtSetTimerResolution");

        if(NtQueryTimerResolution != NULL && NtSetTimerResolution != NULL)
        {
            NtQueryTimerResolution(&MinimumResolution, &MaximumResolution, &ActualResolution);
            if(MaximumResolution != 0)
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
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

