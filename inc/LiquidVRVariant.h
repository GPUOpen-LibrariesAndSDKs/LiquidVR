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
#pragma warning(disable: 4996)

#include "LiquidVR.h"
#include <string>

////////////////////////////////////////////////////////////////////////////////////
#define VARIANT_RETURN_IF_INVALID_POINTER(ptr) if(ptr == NULL){return ALVR_INVALID_ARGUMENT; }

inline ALVR_RESULT ALVR_CDECL_CALL ALVRVariantInit(ALVRVariantStruct* pVariant)
{
    VARIANT_RETURN_IF_INVALID_POINTER(pVariant);
    pVariant->type = ALVR_VARIANT_EMPTY;
    return ALVR_OK;
}
inline ALVR_RESULT ALVR_CDECL_CALL ALVRVariantClear(ALVRVariantStruct* pVariant)
{
    VARIANT_RETURN_IF_INVALID_POINTER(pVariant);
    switch (pVariant->type)
    {
    case ALVR_VARIANT_STRING:
        CoTaskMemFree(pVariant->stringValue);
        break;
    case ALVR_VARIANT_WSTRING:
        CoTaskMemFree(pVariant->wstringValue);
        break;
    case ALVR_VARIANT_INTERFACE:
        if (pVariant->pInterface != NULL)
        {
            pVariant->pInterface->Release();
            pVariant->pInterface = NULL;
        }
        break;

    default:
        break;
    }
    pVariant->type = ALVR_VARIANT_EMPTY;
    return ALVR_OK;
}
inline ALVR_RESULT ALVR_CDECL_CALL ALVRVariantAssignBool(ALVRVariantStruct* pDest, bool value);
inline ALVR_RESULT ALVR_CDECL_CALL ALVRVariantAssignInt64(ALVRVariantStruct* pDest, int64_t value);
inline ALVR_RESULT ALVR_CDECL_CALL ALVRVariantAssignDouble(ALVRVariantStruct* pDest, double value);
inline ALVR_RESULT ALVR_CDECL_CALL ALVRVariantAssignString(ALVRVariantStruct* pDest, const char* value);
inline ALVR_RESULT ALVR_CDECL_CALL ALVRVariantAssignWString(ALVRVariantStruct* pDest, const wchar_t* value);
inline ALVR_RESULT ALVR_CDECL_CALL ALVRVariantAssignInterface(ALVRVariantStruct* pDest, ALVRInterface* value);

inline ALVR_RESULT ALVR_CDECL_CALL ALVRVariantAssignRect(ALVRVariantStruct* pDest, const RECT& value);
inline ALVR_RESULT ALVR_CDECL_CALL ALVRVariantAssignSize(ALVRVariantStruct* pDest, const SIZE& value);
inline ALVR_RESULT ALVR_CDECL_CALL ALVRVariantAssignPoint(ALVRVariantStruct* pDest, const POINT& value);

inline ALVR_RESULT ALVR_CDECL_CALL ALVRVariantCompare(const ALVRVariantStruct* pFirst, const ALVRVariantStruct* pSecond, bool& bEqual)
{
    VARIANT_RETURN_IF_INVALID_POINTER(pFirst);
    VARIANT_RETURN_IF_INVALID_POINTER(pSecond);
    ALVR_RESULT errRet = ALVR_OK;

    if (pFirst == pSecond)
    {
        bEqual = true;
    }
    else if (pFirst->type != pSecond->type)
    {
        bEqual = false;
    }
    else
    {
        switch (pFirst->type)
        {
        case ALVR_VARIANT_EMPTY:
            bEqual = true;
            break;
        case ALVR_VARIANT_BOOL:
            bEqual = pFirst->boolValue == pSecond->boolValue;
            break;
        case ALVR_VARIANT_INT64:
            bEqual = pFirst->int64Value == pSecond->int64Value;
            break;
        case ALVR_VARIANT_DOUBLE:
            bEqual = pFirst->doubleValue == pSecond->doubleValue;
            break;
        case ALVR_VARIANT_RECT:
            bEqual = pFirst->rectValue == pSecond->rectValue;
            break;
        case ALVR_VARIANT_SIZE:
            bEqual = pFirst->sizeValue.cx == pSecond->sizeValue.cx && pFirst->sizeValue.cy == pSecond->sizeValue.cy;
            break;
        case ALVR_VARIANT_POINT:
            bEqual = pFirst->pointValue.x == pSecond->pointValue.x && pFirst->pointValue.y == pSecond->pointValue.y;
            break;
        case ALVR_VARIANT_STRING:
            bEqual = strcmp(pFirst->stringValue, pSecond->stringValue) == 0;
            break;
        case ALVR_VARIANT_WSTRING:
            bEqual = wcscmp(pFirst->wstringValue, pSecond->wstringValue) == 0;
            break;
        case ALVR_VARIANT_INTERFACE:
            bEqual = pFirst->pInterface == pSecond->pInterface;
            break;
        default:
            errRet = ALVR_INVALID_ARGUMENT;
            break;
        }
    }
    return errRet;
}
inline ALVR_RESULT ALVR_CDECL_CALL ALVRVariantCopy(ALVRVariantStruct* pDest, const ALVRVariantStruct* pSrc)
{
    VARIANT_RETURN_IF_INVALID_POINTER(pDest);
    VARIANT_RETURN_IF_INVALID_POINTER(pSrc);
    ALVR_RESULT errRet = ALVR_OK;
    if (pDest != pSrc)
    {
        switch (pSrc->type)
        {
        case ALVR_VARIANT_EMPTY:
            errRet = ALVRVariantClear(pDest);
            break;
        case ALVR_VARIANT_BOOL:
            errRet = ALVRVariantAssignBool(pDest, pSrc->boolValue);
            break;
        case ALVR_VARIANT_INT64:
            errRet = ALVRVariantAssignInt64(pDest, pSrc->int64Value);
            break;
        case ALVR_VARIANT_DOUBLE:
            errRet = ALVRVariantAssignDouble(pDest, pSrc->doubleValue);
            break;
        case ALVR_VARIANT_RECT:
            errRet = ALVRVariantAssignRect(pDest, pSrc->rectValue);
            break;
        case ALVR_VARIANT_SIZE:
            errRet = ALVRVariantAssignSize(pDest, pSrc->sizeValue);
            break;
        case ALVR_VARIANT_POINT:
            errRet = ALVRVariantAssignPoint(pDest, pSrc->pointValue);
            break;
        case ALVR_VARIANT_STRING:
            errRet = ALVRVariantAssignString(pDest, pSrc->stringValue);
            break;
        case ALVR_VARIANT_WSTRING:
            errRet = ALVRVariantAssignWString(pDest, pSrc->wstringValue);
            break;
        case ALVR_VARIANT_INTERFACE:
            errRet = ALVRVariantAssignInterface(pDest, pSrc->pInterface);
            break;
        default:
            errRet = ALVR_INVALID_ARGUMENT;
            break;
        }
    }
    return errRet;
}
inline ALVR_RESULT ALVR_CDECL_CALL ALVRVariantAssignBool(ALVRVariantStruct* pDest, bool value)
{
    VARIANT_RETURN_IF_INVALID_POINTER(pDest);
    ALVR_RESULT errRet = ALVR_OK;

    errRet = ALVRVariantClear(pDest);
    if (errRet == ALVR_OK)
    {
        pDest->type = ALVR_VARIANT_BOOL;
        pDest->boolValue = value;
    }
    return errRet;
}

inline ALVR_RESULT ALVR_CDECL_CALL ALVRVariantAssignInt64(ALVRVariantStruct* pDest, int64_t value)
{
    VARIANT_RETURN_IF_INVALID_POINTER(pDest);
    ALVR_RESULT errRet = ALVR_OK;

    errRet = ALVRVariantClear(pDest);
    if (errRet == ALVR_OK)
    {
        pDest->type = ALVR_VARIANT_INT64;
        pDest->int64Value = value;
    }
    return errRet;
}
inline ALVR_RESULT ALVR_CDECL_CALL ALVRVariantAssignDouble(ALVRVariantStruct* pDest, double value)
{
    VARIANT_RETURN_IF_INVALID_POINTER(pDest);
    ALVR_RESULT errRet = ALVR_OK;

    errRet = ALVRVariantClear(pDest);
    if (errRet == ALVR_OK)
    {
        pDest->type = ALVR_VARIANT_DOUBLE;
        pDest->doubleValue = value;
    }
    return errRet;
}
inline ALVR_RESULT ALVR_CDECL_CALL ALVRVariantAssignString(ALVRVariantStruct* pDest, const char* pValue)
{
    VARIANT_RETURN_IF_INVALID_POINTER(pDest);
    VARIANT_RETURN_IF_INVALID_POINTER(pValue);
    ALVR_RESULT errRet = ALVR_OK;

    errRet = ALVRVariantClear(pDest);
    if (errRet == ALVR_OK)
    {
        pDest->type = ALVR_VARIANT_STRING;
        size_t size = (strlen(pValue) + 1);
        pDest->stringValue = (char*)CoTaskMemAlloc(size * sizeof(char));
        if (pDest->stringValue)
        {
            strncpy(pDest->stringValue, pValue, size);
        }
        else
        {
            errRet = ALVR_OUT_OF_MEMORY;
        }
    }
    return errRet;
}
inline ALVR_RESULT ALVR_CDECL_CALL ALVRVariantAssignWString(ALVRVariantStruct* pDest, const wchar_t* pValue)
{
    VARIANT_RETURN_IF_INVALID_POINTER(pDest);
    VARIANT_RETURN_IF_INVALID_POINTER(pValue);
    ALVR_RESULT errRet = ALVR_OK;

    errRet = ALVRVariantClear(pDest);
    if (errRet == ALVR_OK)
    {
        pDest->type = ALVR_VARIANT_WSTRING;
        size_t size = (wcslen(pValue) + 1);
        pDest->wstringValue = (wchar_t*)CoTaskMemAlloc(size * sizeof(wchar_t));
        if (pDest->wstringValue)
        {
            wcsncpy(pDest->wstringValue, pValue, size);
        }
        else
        {
            errRet = ALVR_OUT_OF_MEMORY;
        }
    }
    return errRet;
}
inline ALVR_RESULT ALVR_CDECL_CALL ALVRVariantAssignInterface(ALVRVariantStruct* pDest, ALVRInterface* pValue)
{
    VARIANT_RETURN_IF_INVALID_POINTER(pDest);
    //VARIANT_RETURN_IF_INVALID_POINTER(pValue);//can be NULL

    ALVR_RESULT errRet = ALVR_OK;
    errRet = ALVRVariantClear(pDest);
    if (errRet == ALVR_OK)
    {
        pDest->type = ALVR_VARIANT_INTERFACE;
        pDest->pInterface = pValue;
        if (pDest->pInterface)
        {
            pDest->pInterface->AddRef();
        }
    }
    return errRet;
}
inline ALVR_RESULT ALVR_CDECL_CALL ALVRVariantAssignRect(ALVRVariantStruct* pDest, const RECT& value)
{
    VARIANT_RETURN_IF_INVALID_POINTER(pDest);
    ALVR_RESULT errRet = ALVR_OK;

    errRet = ALVRVariantClear(pDest);
    if (errRet == ALVR_OK)
    {
        pDest->type = ALVR_VARIANT_RECT;
        pDest->rectValue = value;
    }
    return errRet;
}

inline ALVR_RESULT ALVR_CDECL_CALL ALVRVariantAssignSize(ALVRVariantStruct* pDest, const SIZE& value)
{
    VARIANT_RETURN_IF_INVALID_POINTER(pDest);
    ALVR_RESULT errRet = ALVR_OK;

    errRet = ALVRVariantClear(pDest);
    if (errRet == ALVR_OK)
    {
        pDest->type = ALVR_VARIANT_SIZE;
        pDest->sizeValue = value;
    }
    return errRet;
}
inline ALVR_RESULT ALVR_CDECL_CALL ALVRVariantAssignPoint(ALVRVariantStruct* pDest, const POINT& value)
{
    VARIANT_RETURN_IF_INVALID_POINTER(pDest);
    ALVR_RESULT errRet = ALVR_OK;

    errRet = ALVRVariantClear(pDest);
    if (errRet == ALVR_OK)
    {
        pDest->type = ALVR_VARIANT_POINT;
        pDest->pointValue = value;
    }
    return errRet;
}

//helpers
inline char* ALVR_CDECL_CALL ALVRVariantDuplicateString(const char* from)
{
    char* ret = 0;
    if (from)
    {
        ret = (char*)CoTaskMemAlloc(sizeof(char)*(strlen(from) + 1));
        if (ret)
        {
            strcpy(ret, from);
        }
    }
    return ret;
}

inline void ALVR_CDECL_CALL ALVRVariantFreeString(char* from)
{
    CoTaskMemFree(from);
}

inline wchar_t* ALVR_CDECL_CALL ALVRVariantDuplicateWString(const wchar_t* from)
{
    wchar_t* ret = 0;
    if (from)
    {
        ret = (wchar_t*)CoTaskMemAlloc(sizeof(wchar_t)*(wcslen(from) + 1));
        if (ret)
        {
            wcscpy(ret, from);
        }
    }
    return ret;
}
inline void ALVR_CDECL_CALL ALVRVariantFreeWString(wchar_t* from)
{
    CoTaskMemFree(from);
}

    inline ALVR_VARIANT_TYPE     ALVR_STD_CALL ALVRVariantGetType(const ALVRVariantStruct* _variant) { return (_variant)->type; }
    inline ALVR_VARIANT_TYPE&    ALVR_STD_CALL ALVRVariantGetType(ALVRVariantStruct* _variant) { return (_variant)->type; }
    inline bool                 ALVR_STD_CALL ALVRVariantGetBool(const ALVRVariantStruct* _variant) { return (_variant)->boolValue; }
    inline int64_t              ALVR_STD_CALL ALVRVariantGetInt64(const ALVRVariantStruct* _variant) { return (_variant)->int64Value; }
    inline double           ALVR_STD_CALL ALVRVariantGetDouble(const ALVRVariantStruct* _variant) { return (_variant)->doubleValue; }
    inline const char*          ALVR_STD_CALL ALVRVariantGetString(const ALVRVariantStruct* _variant) { return (_variant)->stringValue; }
    inline const wchar_t*       ALVR_STD_CALL ALVRVariantGetWString(const ALVRVariantStruct* _variant) { return (_variant)->wstringValue; }
    inline const ALVRInterface*  ALVR_STD_CALL ALVRVariantGetInterface(const ALVRVariantStruct* _variant) { return (_variant)->pInterface; }
    inline ALVRInterface*        ALVR_STD_CALL ALVRVariantGetInterface(ALVRVariantStruct* _variant) { return (_variant)->pInterface; }

    inline const RECT&       ALVR_STD_CALL ALVRVariantGetRect (const ALVRVariantStruct* _variant) { return (_variant)->rectValue; }
    inline const SIZE&       ALVR_STD_CALL ALVRVariantGetSize (const ALVRVariantStruct* _variant) { return (_variant)->sizeValue; }
    inline const POINT&       ALVR_STD_CALL ALVRVariantGetPoint(const ALVRVariantStruct* _variant) { return (_variant)->pointValue; }

//inline Variant helper class
    class ALVRVariant : public ALVRVariantStruct
    {
    public:
        class String;
        class WString;

    public:
        ALVRVariant() {  ALVRVariantInit(this); }
        explicit ALVRVariant(const ALVRVariantStruct& other) { ALVRVariantInit(this); ALVRVariantCopy(this, const_cast<ALVRVariantStruct*>(&other)); }

        explicit ALVRVariant(const ALVRVariantStruct* pOther);

        ALVRVariant(const ALVRVariant& other) { ALVRVariantInit(this); ALVRVariantCopy(this, const_cast<ALVRVariantStruct*>(static_cast<const ALVRVariantStruct*>(&other))); }

        explicit inline ALVRVariant(bool value)          { ALVRVariantInit(this); ALVRVariantAssignBool(this, value); }
        explicit inline ALVRVariant(int64_t value)         { ALVRVariantInit(this); ALVRVariantAssignInt64(this, value); }
        explicit inline ALVRVariant(uint64_t value)        { ALVRVariantInit(this); ALVRVariantAssignInt64(this, (int64_t)value); }
        explicit inline ALVRVariant(int32_t value)         { ALVRVariantInit(this); ALVRVariantAssignInt64(this, value); }
        explicit inline ALVRVariant(uint32_t value)        { ALVRVariantInit(this); ALVRVariantAssignInt64(this, value); }
        explicit inline ALVRVariant(double value)        { ALVRVariantInit(this); ALVRVariantAssignDouble(this, value); }
        explicit inline ALVRVariant(const RECT & value)   { ALVRVariantInit(this); ALVRVariantAssignRect(this, value); }
        explicit inline ALVRVariant(const SIZE & value)   { ALVRVariantInit(this); ALVRVariantAssignSize(this, value); }
        explicit inline ALVRVariant(const POINT& value)   { ALVRVariantInit(this); ALVRVariantAssignPoint(this, value); }
        explicit inline ALVRVariant(const char* value)       { ALVRVariantInit(this); ALVRVariantAssignString(this, value); }
        explicit inline ALVRVariant(const wchar_t* value)    { ALVRVariantInit(this); ALVRVariantAssignWString(this, value); }
        explicit inline ALVRVariant(ALVRInterface* pValue)    { ALVRVariantInit(this); ALVRVariantAssignInterface(this, pValue); }

        ~ALVRVariant() { ALVRVariantClear(this); }

        ALVRVariant& operator=(const ALVRVariantStruct& other);
        ALVRVariant& operator=(const ALVRVariantStruct* pOther);
        ALVRVariant& operator=(const ALVRVariant& other);

        ALVRVariant& operator=(bool          value)      { ALVRVariantAssignBool(this, value); return *this;}
        ALVRVariant& operator=(int64_t         value)      { ALVRVariantAssignInt64(this, value); return *this;}
        ALVRVariant& operator=(uint64_t        value)      { ALVRVariantAssignInt64(this, (int64_t)value);  return *this;}
        ALVRVariant& operator=(int32_t         value)      { ALVRVariantAssignInt64(this, value);  return *this;}
        ALVRVariant& operator=(uint32_t        value)      { ALVRVariantAssignInt64(this, value);  return *this;}
        ALVRVariant& operator=(double        value)      { ALVRVariantAssignDouble(this, value);  return *this;}
        ALVRVariant& operator=(const RECT &   value)      { ALVRVariantAssignRect(this, value);  return *this;}
        ALVRVariant& operator=(const SIZE &   value)      { ALVRVariantAssignSize(this, value);  return *this;}
        ALVRVariant& operator=(const POINT&   value)      { ALVRVariantAssignPoint(this, value);  return *this;}
        ALVRVariant& operator=(const char*       value)      { ALVRVariantAssignString(this, value);  return *this;}
        ALVRVariant& operator=(const wchar_t*    value)      { ALVRVariantAssignWString(this, value);  return *this;}
        ALVRVariant& operator=(ALVRInterface*     value)      { ALVRVariantAssignInterface(this, value);  return *this;}

        operator bool() const          { return boolValue;       }
        operator int64_t() const         { return int64Value;      }
        operator uint64_t() const        { return (uint64_t)int64Value; }
        operator int32_t() const         { return (int32_t)int64Value; }
        operator uint32_t() const        { return (uint32_t)int64Value; }
        operator double() const        { return doubleValue;     }
        operator float() const         { return (float)doubleValue; }
        operator RECT () const          { return rectValue;      }
        operator SIZE () const          { return sizeValue; }
        operator POINT() const          { return pointValue;      }
        operator ALVRInterface*() const     { return pInterface;  }
        operator char*() const     { return stringValue; }
        operator wchar_t*() const     { return wstringValue; }

        bool operator==(const ALVRVariantStruct& other) const;
        bool operator==(const ALVRVariantStruct* pOther) const;

        bool operator!=(const ALVRVariantStruct& other) const;
        bool operator!=(const ALVRVariantStruct* pOther) const;

        void Clear() { ALVRVariantClear(this); }

        void Attach(ALVRVariantStruct& variant);
        ALVRVariantStruct Detach();

        ALVRVariantStruct& GetVariant();

        bool Empty() const;
    private:
        template<class ReturnType, ALVR_VARIANT_TYPE variantType, typename Getter>
        ReturnType GetValue(Getter getter) const;
    };
    //-------------------------------------------------------------------------------------------------

    inline ALVRVariant::ALVRVariant(const ALVRVariantStruct* pOther)
    {
        ALVRVariantInit(this);
        if(pOther != NULL)
        {
            ALVRVariantCopy(this, const_cast<ALVRVariantStruct*>(pOther));
        }
    }
    //-------------------------------------------------------------------------------------------------
    template<class ReturnType, ALVR_VARIANT_TYPE variantType, typename Getter>
    ReturnType ALVRVariant::GetValue(Getter getter) const
    {
        ReturnType str = ReturnType();
        if(ALVRVariantGetType(this) == variantType)
        {
            str = static_cast<ReturnType>(getter(this));
        }
        return str;
    }
    //-------------------------------------------------------------------------------------------------
    inline ALVRVariant& ALVRVariant::operator=(const ALVRVariantStruct& other)
    {
        ALVRVariantCopy(this, const_cast<ALVRVariantStruct*>(&other));
        return *this;
    }
    //-------------------------------------------------------------------------------------------------
    inline ALVRVariant& ALVRVariant::operator=(const ALVRVariantStruct* pOther)
    {
        if(pOther != NULL)
        {
            ALVRVariantCopy(this, const_cast<ALVRVariantStruct*>(pOther));
        }
        return *this;
    }
    //-------------------------------------------------------------------------------------------------
    inline ALVRVariant& ALVRVariant::operator=(const ALVRVariant& other)
    {
        ALVRVariantCopy(this,
                const_cast<ALVRVariantStruct*>(static_cast<const ALVRVariantStruct*>(&other)));
        return *this;
    }
    //-------------------------------------------------------------------------------------------------
    inline bool ALVRVariant::operator==(const ALVRVariantStruct& other) const
    {
        return *this == &other;
    }
    //-------------------------------------------------------------------------------------------------
    inline bool ALVRVariant::operator==(const ALVRVariantStruct* pOther) const
    {
        //TODO: double check
        bool ret = false;
        if(pOther == NULL)
        {
            ret = false;
        }
        else
        {
            ALVRVariantCompare(this, pOther, ret);
        }
        return ret;
    }
    //-------------------------------------------------------------------------------------------------
    inline bool ALVRVariant::operator!=(const ALVRVariantStruct& other) const
    {
        return !(*this == &other);
    }
    //-------------------------------------------------------------------------------------------------
    inline bool ALVRVariant::operator!=(const ALVRVariantStruct* pOther) const
    {
        return !(*this == pOther);
    }
    //-------------------------------------------------------------------------------------------------
    inline void ALVRVariant::Attach(ALVRVariantStruct& variant)
    {
        Clear();
        memcpy(this, &variant, sizeof(variant));
        ALVRVariantGetType(&variant) = ALVR_VARIANT_EMPTY;
    }
    //-------------------------------------------------------------------------------------------------
    inline ALVRVariantStruct ALVRVariant::Detach()
    {
        ALVRVariantStruct varResult = *this;
        ALVRVariantGetType(this) = ALVR_VARIANT_EMPTY;
        return varResult;
    }
    //-------------------------------------------------------------------------------------------------
    inline ALVRVariantStruct& ALVRVariant::GetVariant()
    {
        return *static_cast<ALVRVariantStruct*>(this);
    }
    //-------------------------------------------------------------------------------------------------
    inline bool ALVRVariant::Empty() const
    {
        return type == ALVR_VARIANT_EMPTY;
    }

    //----------------------------------------------------------------------------------------------
    // Template methods implementations
    //----------------------------------------------------------------------------------------------
    template<typename _T> inline
        ALVR_RESULT ALVR_STD_CALL ALVRPropertyStorage::SetProperty(const wchar_t* name, const _T& value)
    {
        ALVR_RESULT err = SetProperty(name, static_cast<const ALVRVariantStruct&>(ALVRVariant(value)));
        return err;
    }
    //----------------------------------------------------------------------------------------------
    template<typename _T> inline
        ALVR_RESULT ALVR_STD_CALL ALVRPropertyStorage::GetProperty(const wchar_t* name, _T* pValue) const
    {
        ALVRVariant var;
        ALVR_RESULT err = GetProperty(name, static_cast<ALVRVariantStruct*>(&var));
        if (err == ALVR_OK)
        {
            *pValue = static_cast<_T>(var);
        }
        return err;
    }
    //----------------------------------------------------------------------------------------------
    template<> inline
        ALVR_RESULT ALVR_STD_CALL ALVRPropertyStorage::GetProperty(const wchar_t* name, std::string* pValue) const
    {
        ALVRVariant var;
        ALVR_RESULT err = GetProperty(name, static_cast<ALVRVariantStruct*>(&var));
        if (err == ALVR_OK)
        {
            *pValue = static_cast<char*>(var);
        }
        return err;
    }
    //----------------------------------------------------------------------------------------------
    template<> inline
        ALVR_RESULT ALVR_STD_CALL ALVRPropertyStorage::GetProperty(const wchar_t* name, std::wstring* pValue) const
    {
        ALVRVariant var;
        ALVR_RESULT err = GetProperty(name, static_cast<ALVRVariantStruct*>(&var));
        if (err == ALVR_OK)
        {
            *pValue = static_cast<wchar_t*>(var);
        }
        return err;
    }
    //----------------------------------------------------------------------------------------------
    template<> inline
        ALVR_RESULT ALVR_STD_CALL ALVRPropertyStorage::GetProperty(const wchar_t* name, ALVRInterface** ppValue) const
    {
        ALVRVariant var;
        ALVR_RESULT err = GetProperty(name, static_cast<ALVRVariantStruct*>(&var));
        if (err == ALVR_OK)
        {
            *ppValue = static_cast<ALVRInterface*>(var);
        }
        if (*ppValue)
        {
            (*ppValue)->AddRef();
        }
        return err;
    }