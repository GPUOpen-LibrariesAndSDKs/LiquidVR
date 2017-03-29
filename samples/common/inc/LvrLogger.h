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
#include <string>
#include <memory>
#include <sstream>
#include <ios>
#include "../../../inc/LiquidVR.h"

__int64 high_precision_clock();
__int64 qpc_to_high_precision_clock(__int64 qpc);
void increase_timer_precision();

const wchar_t* ALVRResultToString(ALVR_RESULT res);


enum LogType
{ 
    LogTypeInfo, 
    LogTypeSuccess, 
    LogTypeError,
    LogTypeDefault
};

void WriteLog(const wchar_t* message, LogType type);

#define LOG_WRITE(a, type)\
    do{ \
        std::wstringstream messageStream12345;\
        messageStream12345 << a;\
        WriteLog(messageStream12345.str().c_str(), type);\
    }while(0)

#define LOG_INFO(a) LOG_WRITE(a << std::endl, LogTypeInfo)
#define LOG_SUCCESS(a) LOG_WRITE(a << std::endl, LogTypeSuccess)
#define LOG_ERROR(a) LOG_WRITE(a << std::endl, LogTypeError)

#ifdef _DEBUG
    #define LOG_DEBUG(a)     LOG_INFO(a)
#else
    #define LOG_DEBUG(a)
#endif

#define LOG_ALVR_ERROR(err, text) \
    do{ \
        if( (err) != ALVR_OK) \
        { \
            LOG_WRITE(text << L" Error:" << ALVRResultToString((err)) << std::endl, LogTypeError);\
        } \
    }while(0)

#define CHECK_RETURN(exp, err, text) \
    do{ \
        if((exp) == false) \
        {  \
            LOG_ALVR_ERROR(err, text);\
            return (err); \
        } \
    }while(0)

#define CHECK_ALVR_ERROR_RETURN(err, text) \
    do{ \
        if((err) != ALVR_OK) \
        {  \
            LOG_ALVR_ERROR(err, text);\
            return (err); \
        } \
    }while(0)

#define CHECK_HRESULT_ERROR_RETURN(err, text) \
    do{ \
        if(FAILED(err)) \
        {  \
            LOG_WRITE(text << L" HRESULT Error: " << std::hex << err << std::endl, LogTypeError); \
            return ALVR_FAIL; \
        } \
    }while(0)


