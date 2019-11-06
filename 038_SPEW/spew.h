/*
SPEW OpenGL Function Pointer set-up example. eg. how to replace GLEW in your project
Copyright 2018 Anton Gerdelan <antongdl@protonmail.com>

---------------------------------------------------------------------------------------------------
Instructions
---------------------------------------------------------------------------------------------------
1) include spew.h before including other GL libraries:

```
#include "spew.h"
#include "GLFW/glfw3.h"
```

2) after creating context/window, call SPEW with platform-specfic function loaders:

```
if ( !spew_fptrs( glfwGetProcAddress, glfwExtensionSupported ) ) {
  printf( "SPEW FAILED\n" );
  return false;
}
```

3) include spew.h in files needing to make GL calls

4) add any additional OpenGL functions required to both spew.h and spew.c

*/

#pragma once

// this bit makes sure the includes are not around the wrong way
#if defined(__gl_h_) || defined(__GL_H__) || defined(_GL_H) || defined(__X_GL_H)
#error gl.h included before spew.h
#endif
#if defined(__gl2_h_)
#error gl2.h included before spew.h
#endif
#if defined(__gltypes_h_)
#error gltypes.h included before spew.h
#endif
#if defined(__REGAL_H__)
#error Regal.h included before spew.h
#endif
#if defined(__glext_h_) || defined(__GLEXT_H_)
#error glext.h included before spew.h
#endif
#if defined(__gl_ATI_h_)
#error glATI.h included before spew.h
#endif

#define __gl_h_
#define __gl2_h_
#define __GL_H__
#define _GL_H
#define __gltypes_h_
#define __REGAL_H__
#define __X_GL_H
#define __glext_h_
#define __GLEXT_H_
#define __gl_ATI_h_

#include "GL/glcorearb.h" // local copy of GL Core ARB
#include <stdbool.h>

// i'm adding function pointer externs /as/ i need those functions
// like this
// extern  PFN + ToUppercase(function_name) + PROC
// ref: https://www.opengl.org/discussion_boards/showthread.php/175661-Using-OpenGL-without-GLEW

typedef void ( *SPEWglproc )( void );

// eg should accept ( glfwGetProcAddress, glfwExtensionSupported )
bool spew_fptrs( SPEWglproc ( *get_proc_address )( const char* ), int ( *check_ext_supported )( const char* ) );

// Q. find out why this old 1.1 stuff is fine...worrying
// A. it works if i tell GLFW to include GLCOREARB.h instead of GL.H. otherwise
// i need to comment these out
extern PFNGLENABLEPROC glEnable;
extern PFNGLDISABLEPROC glDisable;
extern PFNGLGETSTRINGPROC glGetString;
extern PFNGLCLEARPROC glClear;
extern PFNGLCLEARCOLORPROC glClearColor;
extern PFNGLPOLYGONMODEPROC glPolygonMode;
extern PFNGLDRAWARRAYSPROC glDrawArrays;
extern PFNGLGETINTEGERVPROC glGetIntegerv;

extern PFNGLGENBUFFERSPROC glGenBuffers;
extern PFNGLBINDBUFFERPROC glBindBuffer;
extern PFNGLBUFFERDATAPROC glBufferData;

extern PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
extern PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
extern PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
extern PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;

extern PFNGLCREATESHADERPROC glCreateShader;
extern PFNGLSHADERSOURCEPROC glShaderSource;
extern PFNGLCOMPILESHADERPROC glCompileShader;

extern PFNGLCREATEPROGRAMPROC glCreateProgram;
extern PFNGLATTACHSHADERPROC glAttachShader;
extern PFNGLLINKPROGRAMPROC glLinkProgram;
extern PFNGLUSEPROGRAMPROC glUseProgram;
extern PFNGLPROGRAMUNIFORM1FPROC glProgramUniform1f;
extern PFNGLPROGRAMUNIFORM2FPROC glProgramUniform2f;
extern PFNGLPROGRAMUNIFORM3FPROC glProgramUniform3f;
extern PFNGLPROGRAMUNIFORM4FPROC glProgramUniform4f;
extern PFNGLPROGRAMUNIFORMMATRIX4FVPROC glProgramUniformMatrix4fv;

extern PFNGLACTIVETEXTUREPROC glActiveTexture;
extern PFNGLBINDTEXTUREPROC glBindTexture;
extern PFNGLTEXPARAMETERIPROC glTexParameteri;

extern PFNGLGENQUERIESPROC glGenQueries;

//-----------------------------extensions---------------------------
extern PFNGLDEBUGMESSAGECALLBACKARBPROC glDebugMessageCallbackARB;
