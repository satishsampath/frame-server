/**
 * Debugmode Frameserver
 * Copyright (C) 2002-2009 Satish Kumar, All Rights Reserved
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

#include <objbase.h>
#include <initguid.h>
#include "SfFioGuids.h"
#include "SfHelpers.h"
#include "ifileio.h"
#include "ifilemng.h"
#include "VegasFS.h"

// {8D9B667F-D109-4855-8C3F-2C5834D10573}
DEFINE_GUID(CLSID_MediaTypeVegasFS,
    0x8d9b667f, 0xd109, 0x4855, 0x8c, 0x3f, 0x2c, 0x58, 0x34, 0xd1, 0x5, 0x73);

// {772CDCA3-C2C0-4ad5-8AC1-06AD475389D6}
DEFINE_GUID(CLSID_VegasFS,
    0x772cdca3, 0xc2c0, 0x4ad5, 0x8a, 0xc1, 0x6, 0xad, 0x47, 0x53, 0x89, 0xd6);

HINSTANCE ghInst;

// ---------------------------------------------------------------------------

class CClassFactory : public IClassFactory {
public:
  CClassFactory(REFCLSID rClsID);

  // IUnknown
  STDMETHODIMP QueryInterface(REFIID riid, void** ppv);
  STDMETHODIMP_(ULONG) AddRef();
  STDMETHODIMP_(ULONG) Release();

  // IClassFactory
  STDMETHODIMP CreateInstance(LPUNKNOWN outer, REFIID riid, void** pv);
  STDMETHODIMP LockServer(BOOL fLock);

private:
  ULONG m_cRef;
  REFCLSID m_rClsID;

  static int m_cLocked;
};

// process-wide dll locked state
int CClassFactory::m_cLocked = 0;

CClassFactory::CClassFactory(REFCLSID rClsID) : m_cRef(0), m_rClsID(rClsID) {
}

STDMETHODIMP CClassFactory::QueryInterface(REFIID riid, void** ppv) {
  *ppv = NULL;
  if ((riid == IID_IUnknown) || (riid == IID_IClassFactory)) {
    *ppv = (void*)this;
    ((IUnknown*)*ppv)->AddRef();
    return S_OK;
  }

  return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CClassFactory::AddRef() {
  return ++m_cRef;
}

STDMETHODIMP_(ULONG) CClassFactory::Release() {
  if (--m_cRef == 0) {
    delete this;
    return 0;
  }
  return m_cRef;
}

STDMETHODIMP CClassFactory::CreateInstance(LPUNKNOWN outer, REFIID riid, void** pv) {
  *pv = NULL;

  HRESULT hr = E_NOINTERFACE;
  if (CLSID_MediaTypeVegasFS == m_rClsID && IID_ISfRenderFileClass == riid) {
    VegasFS* pClass = new VegasFS;
    hr = pClass->QueryInterface(riid, pv);
    if (FAILED(hr))
      delete pClass;
  }

  return hr;
}

STDMETHODIMP CClassFactory::LockServer(BOOL fLock) {
  if (fLock) {
    m_cLocked++;
  } else {
    m_cLocked--;
  }
  return S_OK;
}

// ---------------------------------------------------------------------------

extern "C"
{
STDAPI DllRegisterServer() {
  return SfFileIO_RegisterFilter(
      ghInst, CLSID_MediaTypeVegasFS, APPNAME, "*.avi", APPNAME,
      /*SFFIO_FILTER_TYPE_READ | */ SFFIO_FILTER_TYPE_RENDER,
      SFFIO_STREAM_TYPE_AUDIO | SFFIO_STREAM_TYPE_VIDEO);
}

STDAPI DllUnregisterServer() {
  return SfFileIO_UnregisterFilter(CLSID_MediaTypeVegasFS);
}

STDAPI DllGetClassObject(REFCLSID rClsID, REFIID riid, void** pv) {
  if (!(riid == IID_IUnknown) && !(riid == IID_IClassFactory)) {
    return E_NOINTERFACE;
  }

  *pv = (void*)(IUnknown*)new CClassFactory(rClsID);
  ((IUnknown*)*pv)->AddRef();
  return NOERROR;
}

void DllInitClasses(BOOL bLoading) {
}

STDAPI DllCanUnloadNow() {
  return S_FALSE;
}

BOOL DllInitialize(HINSTANCE hinst) {
  ghInst = hinst;
  return TRUE;
}

BOOL DllTerminate(HINSTANCE hinst) {
  ghInst = NULL;
  return TRUE;
}

BOOL APIENTRY DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved) {
  switch (reason) {
  case DLL_PROCESS_ATTACH:
    DllInitialize(hinst);
    break;
  case DLL_PROCESS_DETACH:
    DllTerminate(hinst);
    break;
  }
  return TRUE;
}
};
