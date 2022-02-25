#define GLEW_STATIC
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <GL/glew.h>
#include <string>

#include "vertex.h"
#include "fragment.h"

#ifdef _DEBUG
#define glCheckError() { \
    GLenum glerr; \
    while ((glerr = glGetError()) != GL_NO_ERROR) { \
        fprintf(stderr, "GL error detected at shaders.cpp %u: %04x\n", __LINE__, glerr); \
        exit(1); \
    } \
}
#else
#define glCheckError()
#endif

GLuint loadShaders() {
	GLuint vsid = glCreateShader(GL_VERTEX_SHADER);
	GLuint fsid = glCreateShader(GL_FRAGMENT_SHADER);
	glCheckError();

	glShaderSource(vsid, 1, &vscode, NULL);
	glCompileShader(vsid);
	glCheckError();

	GLint res;
	int loglen;
	glGetShaderiv(vsid, GL_COMPILE_STATUS, &res);
	glGetShaderiv(vsid, GL_INFO_LOG_LENGTH, &loglen);
	if (loglen > 0) {
		char* message = (char*)malloc((size_t)loglen + 1);
		glGetShaderInfoLog(vsid, loglen, NULL, message);
		printf("Compiling vertex shader: %s\n", message);
		free(message);
	}
	glCheckError();


	glShaderSource(fsid, 1, &fscode, NULL);
	glCompileShader(fsid);
	glCheckError();

	glGetShaderiv(fsid, GL_COMPILE_STATUS, &res);
	glGetShaderiv(fsid, GL_INFO_LOG_LENGTH, &loglen);
	if (loglen > 0) {
		char* message = (char*)malloc((size_t)loglen + 1);
		glGetShaderInfoLog(fsid, loglen, NULL, message);
		printf("Compiling fragment shader: %s\n", message);
		free(message);
	}
	glCheckError();


	GLuint pid = glCreateProgram();
	glCheckError();

	glAttachShader(pid, vsid);
	glCheckError();

	glAttachShader(pid, fsid);
	glCheckError();

	glLinkProgram(pid);
	glCheckError();

	glGetProgramiv(pid, GL_LINK_STATUS, &res);
	glGetProgramiv(pid, GL_INFO_LOG_LENGTH, &loglen);
	if (loglen > 0) {
		char* message = (char*)malloc((size_t)loglen + 1);
		glGetProgramInfoLog(pid, loglen, NULL, message);
		printf("Compiling program: %s\n", message);
		free(message);
	}

	glDetachShader(pid, vsid);
	glDetachShader(pid, fsid);
	glDeleteShader(vsid);
	glDeleteShader(fsid);
	glCheckError();

	return pid;
}