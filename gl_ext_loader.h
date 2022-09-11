static PFNGLCREATESHADERPROC glCreateShader = NULL;
static PFNGLSHADERSOURCEPROC glShaderSource = NULL;
static PFNGLCOMPILESHADERPROC glCompileShader = NULL;
static PFNGLGETSHADERIVPROC glGetShaderiv = NULL;
static PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog = NULL;
static PFNGLCREATEPROGRAMPROC glCreateProgram = NULL;
static PFNGLDELETEPROGRAMPROC glDeleteProgram = NULL;
static PFNGLATTACHSHADERPROC glAttachShader = NULL;
static PFNGLLINKPROGRAMPROC glLinkProgram = NULL;
static PFNGLGETPROGRAMIVPROC glGetProgramiv = NULL;
static PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog = NULL;
static PFNGLDELETESHADERPROC glDeleteShader = NULL;
static PFNGLUSEPROGRAMPROC glUseProgram = NULL;
static PFNGLGENVERTEXARRAYSPROC glGenVertexArrays = NULL;
static PFNGLBINDVERTEXARRAYPROC glBindVertexArray = NULL;
/* static PFNGLDEBUGMESSAGECALLBACKPROC glDebugMessageCallback = NULL; */
static PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation = NULL;
static PFNGLUNIFORM2FPROC glUniform2f = NULL;
static PFNGLUNIFORM2FVPROC glUniform2fv = NULL;
static PFNGLGENBUFFERSPROC glGenBuffers = NULL;
static PFNGLBINDBUFFERPROC glBindBuffer = NULL;
static PFNGLBUFFERDATAPROC glBufferData = NULL;
static PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray = NULL;
static PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer = NULL;
static PFNGLUNIFORM1IPROC glUniform1i = NULL;
static PFNGLUNIFORM1FPROC glUniform1f = NULL;
static PFNGLUNIFORM3FPROC glUniform3f = NULL;
static PFNGLUNIFORM4FPROC glUniform4f = NULL;
static PFNGLUNIFORM3FVPROC glUniform3fv = NULL;
static PFNGLUNIFORM4FVPROC glUniform4fv = NULL;
static PFNGLBUFFERSUBDATAPROC glBufferSubData = NULL;
/* static PFNGLDRAWARRAYSINSTANCEDPROC glDrawArraysInstanced = NULL; */
static PFNGLCLEARBUFFERFVPROC glClearBufferfv = NULL;
static PFNGLVERTEXATTRIB4FVPROC glVertexAttrib4fv = NULL;
static PFNGLGENERATEMIPMAPPROC glGenerateMipmap = NULL;
static PFNGLUNIFORMMATRIX2FVPROC glUniformMatrix2fv = NULL;
static PFNGLUNIFORMMATRIX3FVPROC glUniformMatrix3fv = NULL;
static PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv = NULL;
static PFNGLCREATETEXTURESPROC glCreateTextures = NULL;

static void LoadGLExtensions(void)
{
  // TODO: check for failtures?
  // Maybe some of the functions are not available
  glCreateShader            = (PFNGLCREATESHADERPROC) glXGetProcAddress((const unsigned char *)"glCreateShader");
  glShaderSource            = (PFNGLSHADERSOURCEPROC) glXGetProcAddress((const unsigned char *)"glShaderSource");
  glCompileShader           = (PFNGLCOMPILESHADERPROC) glXGetProcAddress((const unsigned char *)"glCompileShader");
  glGetShaderiv             = (PFNGLGETSHADERIVPROC) glXGetProcAddress((const unsigned char *)"glGetShaderiv");
  glGetShaderInfoLog        = (PFNGLGETSHADERINFOLOGPROC) glXGetProcAddress((const unsigned char *)"glGetShaderInfoLog");
  glAttachShader            = (PFNGLATTACHSHADERPROC) glXGetProcAddress((const unsigned char *)"glAttachShader");
  glCreateProgram           = (PFNGLCREATEPROGRAMPROC) glXGetProcAddress((const unsigned char *)"glCreateProgram");
  glLinkProgram             = (PFNGLLINKPROGRAMPROC) glXGetProcAddress((const unsigned char *)"glLinkProgram");
  glGetProgramiv            = (PFNGLGETPROGRAMIVPROC) glXGetProcAddress((const unsigned char *)"glGetProgramiv");
  glGetProgramInfoLog       = (PFNGLGETPROGRAMINFOLOGPROC) glXGetProcAddress((const unsigned char *)"glGetProgramInfoLog");
  glDeleteShader            = (PFNGLDELETESHADERPROC) glXGetProcAddress((const unsigned char *)"glDeleteShader");
  glUseProgram              = (PFNGLUSEPROGRAMPROC) glXGetProcAddress((const unsigned char *)"glUseProgram");
  glGenVertexArrays         = (PFNGLGENVERTEXARRAYSPROC) glXGetProcAddress((const unsigned char *)"glGenVertexArrays");
  glBindVertexArray         = (PFNGLBINDVERTEXARRAYPROC) glXGetProcAddress((const unsigned char *)"glBindVertexArray");
  glDeleteProgram           = (PFNGLDELETEPROGRAMPROC) glXGetProcAddress((const unsigned char *)"glDeleteProgram");
  glGetUniformLocation      = (PFNGLGETUNIFORMLOCATIONPROC) glXGetProcAddress((const unsigned char *)"glGetUniformLocation");
  glUniform2fv               = (PFNGLUNIFORM2FVPROC) glXGetProcAddress((const unsigned char *)"glUniform2fv");
  glUniform2f               = (PFNGLUNIFORM2FPROC) glXGetProcAddress((const unsigned char *)"glUniform2f");
  glUniform3f               = (PFNGLUNIFORM3FPROC) glXGetProcAddress((const unsigned char *)"glUniform3f");
  glUniform3fv               = (PFNGLUNIFORM3FVPROC) glXGetProcAddress((const unsigned char *)"glUniform3fv");
  glUniform4f               = (PFNGLUNIFORM4FPROC) glXGetProcAddress((const unsigned char *)"glUniform4f");
  glUniform4fv               = (PFNGLUNIFORM4FVPROC) glXGetProcAddress((const unsigned char *)"glUniform4fv");
  glGenBuffers              = (PFNGLGENBUFFERSPROC) glXGetProcAddress((const unsigned char *)"glGenBuffers");
  glBindBuffer              = (PFNGLBINDBUFFERPROC) glXGetProcAddress((const unsigned char *)"glBindBuffer");
  glBufferData              = (PFNGLBUFFERDATAPROC) glXGetProcAddress((const unsigned char *)"glBufferData");
  glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC) glXGetProcAddress((const unsigned char *)"glEnableVertexAttribArray");
  glVertexAttribPointer     = (PFNGLVERTEXATTRIBPOINTERPROC) glXGetProcAddress((const unsigned char *)"glVertexAttribPointer");
  glUniform1i               = (PFNGLUNIFORM1IPROC) glXGetProcAddress((const unsigned char *)"glUniform1i");
  glUniform1f               = (PFNGLUNIFORM1FPROC) glXGetProcAddress((const unsigned char *)"glUniform1f");
  glBufferSubData           = (PFNGLBUFFERSUBDATAPROC) glXGetProcAddress((const unsigned char *)"glBufferSubData");
  glClearBufferfv           = (PFNGLCLEARBUFFERFVPROC) glXGetProcAddress((const unsigned char *)"glClearBufferfv");
  glVertexAttrib4fv         = (PFNGLVERTEXATTRIB4FVPROC) glXGetProcAddress((const unsigned char *)"glVertexAttrib4fv");
  glGenerateMipmap          = (PFNGLGENERATEMIPMAPPROC) glXGetProcAddress((const unsigned char *)"glGenerateMipmap");
  glUniformMatrix2fv        = (PFNGLUNIFORMMATRIX2FVPROC) glXGetProcAddress((const unsigned char *)"glUniformMatrix2fv");
  glUniformMatrix3fv        = (PFNGLUNIFORMMATRIX3FVPROC) glXGetProcAddress((const unsigned char *)"glUniformMatrix3fv");
  glUniformMatrix4fv        = (PFNGLUNIFORMMATRIX4FVPROC) glXGetProcAddress((const unsigned char *)"glUniformMatrix4fv");
  /* glPolygonMode        = (PFNGLPOLYGONMODEPROC) glXGetProcAddress((const unsigned char *)"glPolygonMode"); */
  glCreateTextures          = (PFNGLCREATETEXTURESPROC) glXGetProcAddress((const unsigned char *)"glCreateTextures");

  /* if (glfwExtensionSupported("GL_ARB_debug_output")) { */
  /*   fprintf(stderr, "INFO: ARB_debug_output is supported\n"); */
  /*   glDebugMessageCallback = (PFNGLDEBUGMESSAGECALLBACKPROC) glXGetProcAddress((const unsigned char *)"glDebugMessageCallback"); */
  /* } else { */
  /*   fprintf(stderr, "WARN: ARB_debug_output is NOT supported\n"); */
  /* } */

  /* if (glfwExtensionSupported("GL_EXT_draw_instanced")) { */
  /*   fprintf(stderr, "INFO: EXT_draw_instanced is supported\n"); */
  /*   glDrawArraysInstanced = (PFNGLDRAWARRAYSINSTANCEDPROC) glXGetProcAddress((const unsigned char *)"glDrawArraysInstanced"); */
  /* } else { */
  /*   fprintf(stderr, "WARN: EXT_draw_instanced is NOT supported\n"); */
  /* } */
}
