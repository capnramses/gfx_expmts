#include "spew.h"
#include <stdio.h>

PFNGLENABLEPROC glEnable                                   = NULL; // enable state
PFNGLDISABLEPROC glDisable                                 = NULL;
PFNGLGETSTRINGPROC glGetString                             = NULL; // get
PFNGLGETINTEGERVPROC glGetIntegerv                         = NULL;
PFNGLCLEARPROC glClear                                     = NULL; // viewport
PFNGLCLEARCOLORPROC glClearColor                           = NULL;
PFNGLPOLYGONMODEPROC glPolygonMode                         = NULL; // drawing
PFNGLDRAWARRAYSPROC glDrawArrays                           = NULL;
PFNGLGENBUFFERSPROC glGenBuffers                           = NULL; // buffers
PFNGLBINDBUFFERPROC glBindBuffer                           = NULL;
PFNGLBUFFERDATAPROC glBufferData                           = NULL;
PFNGLGENVERTEXARRAYSPROC glGenVertexArrays                 = NULL; // vertex binding
PFNGLBINDVERTEXARRAYPROC glBindVertexArray                 = NULL;
PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray = NULL;
PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer         = NULL;
PFNGLCREATESHADERPROC glCreateShader                       = NULL; // shaders
PFNGLSHADERSOURCEPROC glShaderSource                       = NULL;
PFNGLCOMPILESHADERPROC glCompileShader                     = NULL;
PFNGLCREATEPROGRAMPROC glCreateProgram                     = NULL;
PFNGLATTACHSHADERPROC glAttachShader                       = NULL;
PFNGLLINKPROGRAMPROC glLinkProgram                         = NULL;
PFNGLUSEPROGRAMPROC glUseProgram                           = NULL;
PFNGLPROGRAMUNIFORM1FPROC glProgramUniform1f               = NULL; // uniforms
PFNGLPROGRAMUNIFORM2FPROC glProgramUniform2f               = NULL;
PFNGLPROGRAMUNIFORM3FPROC glProgramUniform3f               = NULL;
PFNGLPROGRAMUNIFORM4FPROC glProgramUniform4f               = NULL;
PFNGLPROGRAMUNIFORMMATRIX4FVPROC glProgramUniformMatrix4fv = NULL;
PFNGLACTIVETEXTUREPROC glActiveTexture                     = NULL; // textures
PFNGLBINDTEXTUREPROC glBindTexture                         = NULL;
PFNGLTEXPARAMETERIPROC glTexParameteri                     = NULL;
PFNGLGENQUERIESPROC glGenQueries                           = NULL; // queries

// EXTENSIONS
// ----------
PFNGLDEBUGMESSAGECALLBACKARBPROC glDebugMessageCallbackARB = NULL;

bool spew_fptrs( SPEWglproc ( *get_proc_address )( const char* ), int ( *check_ext_supported )( const char* ) ) {
  // format is like this:
  // glCreateProgram =  (PFNGLCREATEPROGRAMPROC)wglGetProcAddress("glCreateProgram");

  glEnable                  = (PFNGLENABLEPROC)get_proc_address( "glEnable" );
  glDisable                 = (PFNGLDISABLEPROC)get_proc_address( "glDisable" );
  glGetString               = (PFNGLGETSTRINGPROC)get_proc_address( "glGetString" );
  glClear                   = (PFNGLCLEARPROC)get_proc_address( "glClear" );
  glClearColor              = (PFNGLCLEARCOLORPROC)get_proc_address( "glClearColor" );
  glPolygonMode             = (PFNGLPOLYGONMODEPROC)get_proc_address( "glPolygonMode" );
  glDrawArrays              = (PFNGLDRAWARRAYSPROC)get_proc_address( "glDrawArrays" );
  glGetIntegerv             = (PFNGLGETINTEGERVPROC)get_proc_address( "glGetIntegerv" );
  glGenBuffers              = (PFNGLGENBUFFERSPROC)get_proc_address( "glGenBuffers" );
  glBindBuffer              = (PFNGLBINDBUFFERPROC)get_proc_address( "glBindBuffer" );
  glBufferData              = (PFNGLBUFFERDATAPROC)get_proc_address( "glBufferData" );
  glGenVertexArrays         = (PFNGLGENVERTEXARRAYSPROC)get_proc_address( "glGenVertexArrays" );
  glBindVertexArray         = (PFNGLBINDVERTEXARRAYPROC)get_proc_address( "glBindVertexArray" );
  glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)get_proc_address( "glEnableVertexAttribArray" );
  glVertexAttribPointer     = (PFNGLVERTEXATTRIBPOINTERPROC)get_proc_address( "glVertexAttribPointer" );
  glCreateShader            = (PFNGLCREATESHADERPROC)get_proc_address( "glCreateShader" );
  glShaderSource            = (PFNGLSHADERSOURCEPROC)get_proc_address( "glShaderSource" );
  glCompileShader           = (PFNGLCOMPILESHADERPROC)get_proc_address( "glCompileShader" );
  glCreateProgram           = (PFNGLCREATEPROGRAMPROC)get_proc_address( "glCreateProgram" );
  glAttachShader            = (PFNGLATTACHSHADERPROC)get_proc_address( "glAttachShader" );
  glLinkProgram             = (PFNGLLINKPROGRAMPROC)get_proc_address( "glLinkProgram" );
  glUseProgram              = (PFNGLUSEPROGRAMPROC)get_proc_address( "glUseProgram" );
  glProgramUniform1f        = (PFNGLPROGRAMUNIFORM1FPROC)get_proc_address( "glProgramUniform1f" );
  glProgramUniform2f        = (PFNGLPROGRAMUNIFORM2FPROC)get_proc_address( "glProgramUniform2f" );
  glProgramUniform3f        = (PFNGLPROGRAMUNIFORM3FPROC)get_proc_address( "glProgramUniform3f" );
  glProgramUniform4f        = (PFNGLPROGRAMUNIFORM4FPROC)get_proc_address( "glProgramUniform4f" );
  glProgramUniformMatrix4fv = (PFNGLPROGRAMUNIFORMMATRIX4FVPROC)get_proc_address( "glProgramUniformMatrix4fv" );
  glBindTexture             = (PFNGLBINDTEXTUREPROC)get_proc_address( "glBindTexture" );
  glTexParameteri           = (PFNGLTEXPARAMETERIPROC)get_proc_address( "glTexParameteri" );
  glGenQueries              = (PFNGLGENQUERIESPROC)get_proc_address( "glGenQueries" );

  // --------------------------- EXTENSIONS ---------------------------------
  // non-core but should be available on a toaster since GL 1.1
  if ( check_ext_supported( "GL_EXT_texture_compression_s3tc" ) ) {
    printf( "EXT_texture_compression_s3tc = yes\n" );
    glDebugMessageCallbackARB = (PFNGLDEBUGMESSAGECALLBACKARBPROC)get_proc_address( "glDebugMessageCallbackARB" );
  } else {
    printf( "EXT_texture_compression_s3tc = no\n" );
  }

  return true;
}
