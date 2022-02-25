#pragma once
#include "framework.h"

#include <stdio.h>
#include <io.h>
#include <ObjIdl.h>
#include <thumbcache.h>
#include <Shlwapi.h>
#include <vector>
#include <GL/glew.h>
#include <glm/glm.hpp>

GLuint loadShaders();
std::vector<glm::vec3>* smlLoadStream(IStream* stream);
HRESULT SMLThumbnails_CreateInstance(REFIID riid, void** ppv);

#ifdef SMLTHUMBNAILS_EXPORTS
#define DECLSPEC __declspec(dllexport)
#else
#define DECLSPEC __declspec(dllimport)
#endif

class DECLSPEC SMLThumbnails : public IThumbnailProvider, IInitializeWithStream {
protected:
    IStream* stream;
    long refs;

public:
    SMLThumbnails();
    ~SMLThumbnails();
    HRESULT Initialize(IStream* pstream, DWORD grfMode);
    HRESULT GetThumbnail(UINT cx, HBITMAP* phbmp, WTS_ALPHATYPE* pdwAlpha);
    HRESULT renderImage(UINT cx, uint32_t** image);

    IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv);
    IFACEMETHODIMP_(ULONG) AddRef();
    IFACEMETHODIMP_(ULONG) Release();
};

class FileStream : public IStream {
protected:
    FILE* fp;

    FileStream(FILE* _fp) {
        fp = _fp;
    }

public:
    FileStream(const char* fn) {
        WCHAR* filename = new WCHAR[(strlen(fn) * 2 + 8)];
        wsprintf(filename, L"\\\\?\\%S", fn);

        _wfopen_s(&fp, filename, L"rb");
        if (!fp) {
            fprintf(stderr, "Error opening '%ls' for read.\n", filename);
            exit(2);
        }

        delete[] filename;
    }
    ~FileStream() {
        fclose(fp);
    }

    HRESULT Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER* plibNewPosition) {
        return fseek(fp, (long)(dlibMove.QuadPart), dwOrigin);
    }
    HRESULT SetSize(ULARGE_INTEGER libNewSize) { return E_NOTIMPL; }
    HRESULT CopyTo(IStream* pstm, ULARGE_INTEGER cb, ULARGE_INTEGER* pcbRead, ULARGE_INTEGER* pcbWritten) { return E_NOTIMPL; }
    HRESULT Commit(DWORD grfCommitFlags) { return E_NOTIMPL; }
    HRESULT Revert(void) { return E_NOTIMPL; }
    HRESULT LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) { return E_NOTIMPL; }
    HRESULT UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) { return E_NOTIMPL; }
    HRESULT Stat(STATSTG* pstatstg, DWORD grfStatFlag) { return E_NOTIMPL; }
    HRESULT Clone(IStream** ppstm) {
        *ppstm = new FileStream(_fdopen(_dup(_fileno(fp)), "rb"));
        return S_OK;
    }

    HRESULT Read(void* pv, ULONG cb, ULONG* pcbRead) {
        size_t bytes = fread(pv, 1, cb, fp);
        *pcbRead = (ULONG)bytes;
        return S_OK;
    }
    HRESULT Write(const void* pv, ULONG cb, ULONG* pcbWritten) { return E_NOTIMPL; }

    HRESULT QueryInterface(REFIID riid, void** ppvObject) { return E_NOTIMPL; }
    ULONG AddRef(void) { return E_NOTIMPL; }
    ULONG Release(void) { return E_NOTIMPL; }
};