/**
 * Debugmode Frameserver
 * Copyright (C) 2002-2019 Satish Kumar, All Rights Reserved
 * http://www.debugmode.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <TCHAR.H>
#include <objbase.h>
#include <initguid.h>
#include "SfBase.h"
#include "SfEnum.h"
#include "SfMem.h"
#include "SfUnsharedFIOPluginInfo.h"
#include "VegasFS.h"

#pragma warning (disable : 4100)

// For filemgr querying info from non-shared plugins
// {8D9B667F-D109-4855-8C3F-2C5834D10574}
DEFINE_GUID(CLSID_SfMediaGetUnsharedTypeInfo,
    0xf6e48972, 0x515b, 0x4a5e, 0xb3, 0x16, 0xa4, 0x6a, 0x3, 0xc9, 0xc3, 0xc0);

DEFINE_GUID(CLSID_ExampleMediaType,
    0x8d9b667f, 0xd109, 0x4855, 0x8c, 0x3f, 0x2c, 0x58, 0x34, 0xd1, 0x05, 0x74);

HINSTANCE ghInst;

// ---------------------------------------------------------------------------

class PluginInfo : public ISfUnsharedFIOPluginInfo {
public:
  PluginInfo();
  ~PluginInfo();

  STDMETHOD(QueryInterface) (REFIID riid, void** ppvObj);
  STDMETHOD_(ULONG, AddRef) ();
  STDMETHOD_(ULONG, Release) ();

  // ISfUnsharedFIOPluginInfo methods
  STDMETHOD(Init) (LPCWSTR key, DWORD appGrade, BOOL demo);
  STDMETHOD(GetFilterCount) (LONG * count);
  STDMETHOD(GetFilterInfo) (LONG index, GUID * uuid, LPWSTR description,
      UINT cchDescription, DWORD * access, DWORD * streamTypes, DWORD * appLevel,
      DWORD * appLevel2, DWORD * appLevel3, DWORD * appLevel4, DWORD * expireDate,
      DWORD * flags);

private:
  ULONG m_cRef;
};

PluginInfo::PluginInfo() : m_cRef(0) {
}

PluginInfo::~PluginInfo() {
}

STDMETHODIMP PluginInfo::QueryInterface(REFIID riid, LPVOID* ppvObj) {
  if (__uuidof(ISfUnsharedFIOPluginInfo) == riid || IID_IUnknown == riid) {
    AddRef();
    *ppvObj = static_cast<ISfUnsharedFIOPluginInfo*>(this);
    return S_OK;
  }
  return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) PluginInfo::AddRef() {
  return InterlockedIncrement((long*)&m_cRef);
}

STDMETHODIMP_(ULONG) PluginInfo::Release() {
  LONG cRef = InterlockedDecrement((long*)&m_cRef);
  if(cRef == 0)
  {
    SfDelete this;
  }
  return cRef;
}

STDMETHODIMP PluginInfo::Init(LPCWSTR key, DWORD appGrade, BOOL demo) {
  return S_OK;
}

STDMETHODIMP PluginInfo::GetFilterCount(LONG* count) {
  if (!count)
    return E_INVALIDARG;
  *count = 1;
  return S_OK;
}

STDMETHODIMP PluginInfo::GetFilterInfo(LONG index, GUID* uuid, LPWSTR description,
    UINT cchDescription, DWORD* access, DWORD* streamTypes, DWORD* appLevel,
    DWORD* appLevel2, DWORD* appLevel3, DWORD* appLevel4, DWORD* expireDate, DWORD* flags) {
  if (index != 0)
    return E_INVALIDARG;

  if (description) wcsncpy(description, L"DebugMode FrameServer", cchDescription);
  if (uuid) *uuid = CLSID_ExampleMediaType;
  if (access) *access = SFFIO_FILTER_TYPE_RENDER;
  if (streamTypes) *streamTypes = SFFIO_STREAM_TYPE_AUDIO | SFFIO_STREAM_TYPE_VIDEO;
  if (appLevel) *appLevel = ULONG_MAX;
  if (appLevel2) *appLevel2 = ULONG_MAX;
  if (appLevel3) *appLevel3 = ULONG_MAX;
  if (appLevel4) *appLevel4 = ULONG_MAX;
  if (flags) *flags = SFFIO_PLUGINFLAGS_LOAD_ON_INIT;
  return S_OK;
}

// ---------------------------------------------------------------------------

class CClassFactory : public IClassFactory {
public:
  CClassFactory(REFCLSID rClsID);

  // IUnknown
  STDMETHOD(QueryInterface) (REFIID riid, void** ppv);
  STDMETHOD_(ULONG, AddRef) ();
  STDMETHOD_(ULONG, Release) ();

  // IClassFactory
  STDMETHOD(CreateInstance) (LPUNKNOWN outer, REFIID riid, void** pv);
  STDMETHOD(LockServer) (BOOL fLock);

private:
  ULONG m_cRef;
  REFCLSID m_rclsid;

  static long m_cLocked;
};

// process-wide dll locked state
long CClassFactory::m_cLocked = 0;

CClassFactory::CClassFactory(REFCLSID rclsid) : m_cRef(1), m_rclsid(rclsid) {
}

STDMETHODIMP CClassFactory::QueryInterface(REFIID riid, void** ppv) {
  if (IsEqualIID(IID_IClassFactory, riid) || IsEqualIID(IID_IUnknown, riid)) {
    *ppv = static_cast<IClassFactory*>(this);
    static_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK;
  }

  *ppv = NULL;
  return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CClassFactory::AddRef() {
  return InterlockedIncrement((long*)&m_cRef);
}

STDMETHODIMP_(ULONG) CClassFactory::Release() {
  LONG cRef = InterlockedDecrement((long*)&m_cRef);
  if(cRef == 0)
  {
    SfDelete this;
  }
  return cRef;
}

STDMETHODIMP CClassFactory::CreateInstance(LPUNKNOWN outer, REFIID riid, void** ppv) {
  if (NULL == ppv)
    return E_POINTER;

  HRESULT hr = E_NOINTERFACE;
  *ppv = NULL;

  if (CLSID_ExampleMediaType == m_rclsid) {
    if (IsEqualIID(__uuidof(ISfRenderFileClass), riid)) {
      VegasFS* fs = SfNew VegasFS;
      if (fs) {
        hr = fs->QueryInterface(riid, ppv);
      } else {
        hr = E_OUTOFMEMORY;
      }
    }
  } else if (CLSID_SfMediaGetUnsharedTypeInfo == m_rclsid) {
    if (IsEqualIID(__uuidof(ISfUnsharedFIOPluginInfo), riid) ||
        IsEqualIID(IID_IUnknown, riid)) {
      PluginInfo* pClass = SfNew PluginInfo;
      if (pClass) {
        hr = pClass->QueryInterface(riid, ppv);
        if (FAILED(hr))
		      pClass->Release();
      } else {
        hr = E_OUTOFMEMORY;
      }
    }
  }

  return hr;
}

STDMETHODIMP CClassFactory::LockServer(BOOL fLock) {
  if (fLock) {
    InterlockedIncrement(&m_cLocked);
  } else {
    InterlockedDecrement(&m_cLocked);
  }
  return S_OK;
}

// ---------------------------------------------------------------------------

extern "C" {
BOOL APIENTRY DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved) {
  if (reason == DLL_PROCESS_ATTACH) {
    ghInst = hinst;
    DisableThreadLibraryCalls(hinst);
  }
  return TRUE;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppv) {
  HRESULT hr = E_NOINTERFACE;

  *ppv = NULL;

  if (IsEqualIID(IID_IClassFactory, riid) || IsEqualIID(IID_IUnknown, riid)) {
    CClassFactory* pFactory = SfNew CClassFactory(rclsid);
    if(pFactory) {
      hr = pFactory->QueryInterface(riid, ppv);
      pFactory->Release();
    } else {
      hr = E_OUTOFMEMORY;
    }
  }
  return hr;
}

STDAPI DllCanUnloadNow() {
  return S_FALSE;
}

void DllInitClasses(BOOL bLoading) {
}

STDAPI DllRegisterServer() {
  return E_NOTIMPL;
}

STDAPI DllUnregisterServer() {
  return E_NOTIMPL;
}
} // extern "C"
