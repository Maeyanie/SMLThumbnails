#define GLEW_STATIC

#include <stdio.h>
#include <stdlib.h>
#include <thumbcache.h>
#include <ObjIdl.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
using namespace glm;
#include <vector>
#include "SMLThumbnails.h"
using namespace std;

#pragma comment(lib, "glew32s.lib")
#pragma comment(lib, "glfw3_mt.lib")
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "shlwapi.lib")

HRESULT SMLThumbnails_CreateInstance(REFIID riid, void** ppv) {
    SMLThumbnails* pNew = new (std::nothrow) SMLThumbnails();
    HRESULT hr = pNew ? S_OK : E_OUTOFMEMORY;
    if (SUCCEEDED(hr)) {
        hr = pNew->QueryInterface(riid, ppv);
        pNew->Release();
    }
    return hr;
}

// IUnknown
IFACEMETHODIMP SMLThumbnails::QueryInterface(REFIID riid, void** ppv) {
    static const QITAB qit[] = {
        QITABENT(SMLThumbnails, IInitializeWithStream),
        QITABENT(SMLThumbnails, IThumbnailProvider),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}
IFACEMETHODIMP_(ULONG) SMLThumbnails::AddRef() {
    return InterlockedIncrement(&refs);
}
IFACEMETHODIMP_(ULONG) SMLThumbnails::Release() {
    ULONG cRef = InterlockedDecrement(&refs);
    if (cRef == 0) {
        delete this;
    }
    return cRef;
}



SMLThumbnails::SMLThumbnails() {
    stream = NULL;
    refs = 1;
}
SMLThumbnails::~SMLThumbnails() {
    if (stream) stream->Release();
}

HRESULT SMLThumbnails::Initialize(IStream* pstream, DWORD grfMode) {
    if (stream) return HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);
    return pstream->QueryInterface(&stream);
}

HRESULT SMLThumbnails::GetThumbnail(UINT cx, HBITMAP* phbmp, WTS_ALPHATYPE* pdwAlpha) {
    *pdwAlpha = WTSAT_RGB;

    uint32_t* image = new uint32_t[cx * cx];
    HRESULT ret = renderImage(cx, &image);
    if (ret != S_OK) return ret;

    uint32_t* flippedImage = new uint32_t[cx * cx];
    for (int y = 0; y < cx; y++) {
        uint32_t io = y * cx;
        uint32_t oo = (cx * (cx-1)) - io;
        memcpy(flippedImage + oo, image + io, cx * 4);
    }

    HBITMAP hbmp = CreateBitmap(cx, cx, 1, 32, flippedImage);
    assert(hbmp != NULL);
    *phbmp = hbmp;
    delete[] image;
    delete[] flippedImage;

    return S_OK;
}

#ifdef _DEBUG
#define glCheckError() { \
    GLenum glerr; \
    while ((glerr = glGetError()) != GL_NO_ERROR) { \
        fprintf(stderr, "GL error detected at %u: %04x\n", __LINE__, glerr); \
        exit(1); \
    } \
}
#else
#define glCheckError()
#endif

void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id,
    GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
    if (type == GL_DEBUG_TYPE_ERROR) {
        fprintf(stderr, "\n** GL ERROR ** type = 0x%x, severity = 0x%x\n%s\n\n", type, severity, message);
        return;
    } else {
        fprintf(stderr, "\nGL DEBUG: type = 0x%x, severity = 0x%x\n%s\n\n", type, severity, message);
        return;
    }
}


HRESULT SMLThumbnails::renderImage(UINT cx, uint32** image) {
    if (glfwInit() == GLFW_FALSE) {
        fprintf(stderr, "GLFW init failed.\n");
        return E_FAIL;
    }

    glfwWindowHint(GLFW_SAMPLES, 8);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(cx, cx, "SMLThumbnails", NULL, NULL);
    if (!window) {
        fprintf(stderr, "Could not open a GLFW window for OpenGL 3.3.\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    glewExperimental = true;
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "GLEW init failed.\n");
        return E_FAIL;
    }

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(MessageCallback, 0);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCheckError();



    GLuint va;
    glGenVertexArrays(1, &va);
    glBindVertexArray(va);

    GLuint pid = loadShaders();



    vector<vec3>* model = smlLoadStream(stream);
    size_t vCount = model->size();
    GLfloat* vList = new GLfloat[vCount * 3]();
    for (size_t i = 0; i < vCount; i++) {
        vList[i * 3] = model->at(i).x;
        vList[i * 3 + 1] = model->at(i).y;
        vList[i * 3 + 2] = model->at(i).z;
    }

    GLuint vb;
    glGenBuffers(1, &vb);
    glBindBuffer(GL_ARRAY_BUFFER, vb);
    glBufferData(GL_ARRAY_BUFFER, vCount * 12, vList, GL_STATIC_DRAW);



    vec3 min, max;
    min = max = (*model)[0];
    for (int i = 0; i < vCount; i++) {
        for (int j = 0; j < 3; j++) {
            if ((*model)[i][j] < min[j]) min[j] = (*model)[i][j];
            if ((*model)[i][j] > max[j]) max[j] = (*model)[i][j];
        }
    }

    vec3 center = max + min;
    center /= 2;
    printf("min=%f %f %f\nmax=%f %f %f\ncenter=%f %f %f\n",
        min.x, min.y, min.z, max.x, max.y, max.z, center.x, center.y, center.z);

    vec3 size = max - min;
    float longest = size.x;
    if (size.y > longest) longest = size.y;
    if (size.z > longest) longest = size.z;
    printf("Longest side: %f\n", longest);
    vec4 resize = { 1.0 / longest, 1.0 / longest, 1.0 / longest, 1 };

    mat4 mModel = translate(mat4(1.0f), -center);
    mModel *= resize;

    mat4 mView = lookAt(vec3(5, -5, 5), vec3(0, 0, 0), vec3(0, 0, 1));

    const float zoom = 0.85f;
    mat4 mProjection = ortho(-zoom, zoom, -zoom, zoom, -10.0f, 10.0f);

    mat4 mvp = mProjection * mView * mModel;

    GLuint mvpID = glGetUniformLocation(pid, "MVP");
    GLuint mID = glGetUniformLocation(pid, "M");
    GLuint vID = glGetUniformLocation(pid, "V");

    glUseProgram(pid);



    GLuint fb = 0;
    glGenFramebuffers(1, &fb);
    glBindFramebuffer(GL_FRAMEBUFFER, fb);

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, cx, cx, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    GLuint db;
    glGenRenderbuffers(1, &db);
    glBindRenderbuffer(GL_RENDERBUFFER, db);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, cx, cx);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, db);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tex, 0);
    GLenum drawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, drawBuffers);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "Failed initializing render-to-texture.\n");
        return E_FAIL;
    }
    glCheckError();



    printf("Beginning render.\n");
    glBindFramebuffer(GL_FRAMEBUFFER, fb);
    glCheckError();
    glViewport(0, 0, cx, cx);
    glCheckError();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(pid);

    glUniformMatrix4fv(mvpID, 1, GL_FALSE, &mvp[0][0]);
    glUniformMatrix4fv(mID, 1, GL_FALSE, &mModel[0][0]);
    glUniformMatrix4fv(vID, 1, GL_FALSE, &mView[0][0]);



    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vb);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(vCount * 3));
    glCheckError();

    glDisableVertexAttribArray(0);
    glCheckError();

    printf("Render complete.\n");

    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, *image);
    glCheckError();

    glfwTerminate();
    return S_OK;
}

