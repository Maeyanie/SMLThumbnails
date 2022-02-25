// dllmain.cpp : Defines the entry point for the DLL application.
#include "SMLThumbnails.h"
#include <objbase.h>
#include <shlwapi.h>
#include <thumbcache.h> // For IThumbnailProvider.
#include <shlobj.h>     // For SHChangeNotify

#define SZ_CLSID_SMLTHUMBHANDLER     L"{44569c40-4d15-4cd9-afae-b99a95acea2a}"
#define SZ_SMLTHUMBHANDLER           L"SML Thumbnail Handler"

const CLSID CLSID_SMLThumbHandler = { 0x44569c40, 0x4d15, 0x4cd9, {0xaf, 0xae, 0xb9, 0x9a, 0x95, 0xac, 0xea, 0x2a} };

typedef HRESULT(*PFNCREATEINSTANCE)(REFIID riid, void** ppvObject);
struct CLASS_OBJECT_INIT {
    const CLSID* pClsid;
    PFNCREATEINSTANCE pfnCreate;
};

// add classes supported by this module here
const CLASS_OBJECT_INIT c_rgClassObjectInit[] = {
    { &CLSID_SMLThumbHandler, SMLThumbnails_CreateInstance }
};

long g_cRefModule = 0;
HINSTANCE g_hInst = NULL;

__declspec(dllexport) BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        g_hInst = hModule;
        DisableThreadLibraryCalls(hModule);
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

__control_entrypoint(DllExport) STDAPI DllCanUnloadNow() {
    return (g_cRefModule == 0) ? S_OK : S_FALSE;
}
__control_entrypoint(DllExport) void DllAddRef() {
    InterlockedIncrement(&g_cRefModule);
}
__control_entrypoint(DllExport) void DllRelease() {
    InterlockedDecrement(&g_cRefModule);
}

class CClassFactory : public IClassFactory {
public:
    static HRESULT CreateInstance(REFCLSID clsid, const CLASS_OBJECT_INIT* pClassObjectInits, size_t cClassObjectInits, REFIID riid, void** ppv) {
        *ppv = NULL;
        HRESULT hr = CLASS_E_CLASSNOTAVAILABLE;
        for (size_t i = 0; i < cClassObjectInits; i++) {
            if (clsid == *pClassObjectInits[i].pClsid) {
                IClassFactory* pClassFactory = new (std::nothrow) CClassFactory(pClassObjectInits[i].pfnCreate);
                hr = pClassFactory ? S_OK : E_OUTOFMEMORY;
                if (SUCCEEDED(hr)) {
                    hr = pClassFactory->QueryInterface(riid, ppv);
                    pClassFactory->Release();
                }
                break; // match found
            }
        }
        return hr;
    }

    CClassFactory(PFNCREATEINSTANCE pfnCreate) : _cRef(1), _pfnCreate(pfnCreate) {
        DllAddRef();
    }

    // IUnknown
    IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv) {
        static const QITAB qit[] = { 
            QITABENT(CClassFactory, IClassFactory),
            { 0 }
        };
        return QISearch(this, qit, riid, ppv);
    }
    IFACEMETHODIMP_(ULONG) AddRef() {
        return InterlockedIncrement(&_cRef);
    }
    IFACEMETHODIMP_(ULONG) Release() {
        long cRef = InterlockedDecrement(&_cRef);
        if (cRef == 0) delete this;
        return cRef;
    }

    // IClassFactory
    IFACEMETHODIMP CreateInstance(IUnknown* punkOuter, REFIID riid, void** ppv) {
        return punkOuter ? CLASS_E_NOAGGREGATION : _pfnCreate(riid, ppv);
    }
    IFACEMETHODIMP LockServer(BOOL fLock) {
        if (fLock) DllAddRef();
        else DllRelease();
        return S_OK;
    }

private:
    ~CClassFactory() {
        DllRelease();
    }

    long _cRef;
    PFNCREATEINSTANCE _pfnCreate;
};

_Check_return_ STDAPI DllGetClassObject(_In_ REFCLSID clsid, _In_ REFIID riid, _Outptr_ LPVOID FAR* ppv) {
    return CClassFactory::CreateInstance(clsid, c_rgClassObjectInit, ARRAYSIZE(c_rgClassObjectInit), riid, ppv);
}

// A struct to hold the information required for a registry entry
struct REGISTRY_ENTRY {
    HKEY   hkeyRoot;
    PCWSTR pszKeyName;
    PCWSTR pszValueName;
    PCWSTR pszData;
};

// Creates a registry key (if needed) and sets the default value of the key
HRESULT CreateRegKeyAndSetValue(const REGISTRY_ENTRY* pRegistryEntry) {
    HKEY hKey;
    HRESULT hr = HRESULT_FROM_WIN32(RegCreateKeyExW(pRegistryEntry->hkeyRoot, pRegistryEntry->pszKeyName,
        0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, NULL));
    if (SUCCEEDED(hr)) {
        hr = HRESULT_FROM_WIN32(RegSetValueExW(hKey, pRegistryEntry->pszValueName, 0, REG_SZ,
            (LPBYTE)pRegistryEntry->pszData,
            ((DWORD)wcslen(pRegistryEntry->pszData) + 1) * sizeof(WCHAR)));
        RegCloseKey(hKey);
    }
    return hr;
}

//
// Registers this COM server
// regsvr32 E:\Devel\SMLThumbnails\SMLThumbnails\x64\Debug\SMLThumbnails.dll
// regsvr32 /u E:\Devel\SMLThumbnails\SMLThumbnails\x64\Debug\SMLThumbnails.dll
STDAPI DllRegisterServer() {
    HRESULT hr;
    WCHAR szModuleName[MAX_PATH];

    if (!GetModuleFileNameW(g_hInst, szModuleName, ARRAYSIZE(szModuleName))) {
        hr = HRESULT_FROM_WIN32(GetLastError());
    } else {
        // List of registry entries we want to create
        const REGISTRY_ENTRY rgRegistryEntries[] = {
            // RootKey            KeyName                                                                           ValueName               Data
            {HKEY_CURRENT_USER,   L"Software\\Classes\\CLSID\\" SZ_CLSID_SMLTHUMBHANDLER,                           NULL,                   SZ_SMLTHUMBHANDLER},
            {HKEY_CURRENT_USER,   L"Software\\Classes\\CLSID\\" SZ_CLSID_SMLTHUMBHANDLER L"\\InProcServer32",       NULL,                   szModuleName},
            {HKEY_CURRENT_USER,   L"Software\\Classes\\CLSID\\" SZ_CLSID_SMLTHUMBHANDLER L"\\InProcServer32",       L"ThreadingModel",      L"Apartment"},
            {HKEY_CURRENT_USER,   L"Software\\Classes\\.sml\\ShellEx\\{e357fccd-a995-4576-b01f-234630154e96}",      NULL,                   SZ_CLSID_SMLTHUMBHANDLER},
        };

        hr = S_OK;
        for (int i = 0; i < ARRAYSIZE(rgRegistryEntries) && SUCCEEDED(hr); i++) {
            hr = CreateRegKeyAndSetValue(&rgRegistryEntries[i]);
        }
    }
    if (SUCCEEDED(hr)) {
        // This tells the shell to invalidate the thumbnail cache.  This is important because any .recipe files
        // viewed before registering this handler would otherwise show cached blank thumbnails.
        SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
    }
    return hr;
}

//
// Unregisters this COM server
//
STDAPI DllUnregisterServer() {
    HRESULT hr = S_OK;

    const PCWSTR rgpszKeys[] = {
        L"Software\\Classes\\CLSID\\" SZ_CLSID_SMLTHUMBHANDLER,
        L"Software\\Classes\\.sml\\ShellEx\\{e357fccd-a995-4576-b01f-234630154e96}"
    };

    // Delete the registry entries
    for (int i = 0; i < ARRAYSIZE(rgpszKeys) && SUCCEEDED(hr); i++) {
        hr = HRESULT_FROM_WIN32(RegDeleteTreeW(HKEY_CURRENT_USER, rgpszKeys[i]));
        if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
            // If the registry entry has already been deleted, say S_OK.
            hr = S_OK;
        }
    }
    return hr;
}

