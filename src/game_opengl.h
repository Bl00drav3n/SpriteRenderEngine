// NOTE: define GL_MACRO if you only want to assign function pointers
// for example: #define GL_MACRO(name, ...) name = (PF_##name*)Load(#name)

#ifndef GL_MACRO

#ifdef _WIN32
#define APIENTRY __stdcall
#else
#define APIENTRY __cdecl
#endif

typedef unsigned int GLenum;
typedef unsigned char GLboolean;
typedef unsigned int GLbitfield;
typedef signed char GLbyte;
typedef short GLshort;
typedef int GLint;
typedef int GLsizei;

typedef unsigned char GLubyte;
typedef unsigned short GLushort;
typedef unsigned int GLuint;
typedef float GLfloat;
typedef float GLclampf;
typedef double GLdouble;
typedef double GLclampd;
typedef void GLvoid;
typedef char GLchar; 
typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;

#define GL_FALSE 0
#define GL_TRUE 1

#define GL_DEPTH_BUFFER_BIT               0x00000100
#define GL_STENCIL_BUFFER_BIT             0x00000400
#define GL_COLOR_BUFFER_BIT               0x00004000
#define GL_SCISSOR_BIT                    0x00080000

#define GL_POINTS                         0x0000
#define GL_LINES                          0x0001
#define GL_LINE_LOOP                      0x0002
#define GL_LINE_STRIP                     0x0003
#define GL_TRIANGLES                      0x0004
#define GL_TRIANGLE_STRIP                 0x0005
#define GL_TRIANGLE_FAN                   0x0006
#define GL_SRC_COLOR                      0x0300
#define GL_ONE_MINUS_SRC_COLOR            0x0301
#define GL_SRC_ALPHA                      0x0302
#define GL_ONE_MINUS_SRC_ALPHA            0x0303
#define GL_DST_ALPHA                      0x0304
#define GL_ONE_MINUS_DST_ALPHA            0x0305
#define GL_BLEND                          0x0BE2
#define GL_TEXTURE_2D                     0x0DE1
#define GL_TEXTURE_2D_ARRAY               0x8C1A
#define GL_TEXTURE_BUFFER                 0x8C2A

#define GL_NEVER                          0x0200
#define GL_LESS                           0x0201
#define GL_EQUAL                          0x0202
#define GL_LEQUAL                         0x0203
#define GL_GREATER                        0x0204
#define GL_NOTEQUAL                       0x0205
#define GL_GEQUAL                         0x0206
#define GL_ALWAYS                         0x0207

#define GL_DEPTH_TEST                     0x0B71

#define GL_CLAMP_TO_EDGE                  0x812F
#define GL_BYTE                           0x1400
#define GL_UNSIGNED_BYTE                  0x1401
#define GL_SHORT                          0x1402
#define GL_UNSIGNED_SHORT                 0x1403
#define GL_INT                            0x1404
#define GL_UNSIGNED_INT                   0x1405
#define GL_FLOAT                          0x1406
#define GL_2_BYTES                        0x1407
#define GL_3_BYTES                        0x1408
#define GL_4_BYTES                        0x1409
#define GL_DOUBLE                         0x140A
#define GL_MODELVIEW                      0x1700
#define GL_PROJECTION                     0x1701
#define GL_RGBA                           0x1908
#define GL_MODULATE                       0x2100
#define GL_TEXTURE_ENV_MODE               0x2200
#define GL_TEXTURE_ENV                    0x2300
#define GL_NEAREST                        0x2600
#define GL_LINEAR                         0x2601
#define GL_TEXTURE_MAG_FILTER             0x2800
#define GL_TEXTURE_MIN_FILTER             0x2801
#define GL_TEXTURE_WRAP_S                 0x2802
#define GL_TEXTURE_WRAP_T                 0x2803
#define GL_REPEAT                         0x2901
#define GL_ZERO                           0
#define GL_ONE                            1
#define GL_SRC_COLOR                      0x0300
#define GL_ONE_MINUS_SRC_COLOR            0x0301
#define GL_SRC_ALPHA                      0x0302
#define GL_ONE_MINUS_SRC_ALPHA            0x0303
#define GL_DST_ALPHA                      0x0304
#define GL_ONE_MINUS_DST_ALPHA            0x0305
#define GL_RGBA8                          0x8058
#define GL_R8                             0x8229
#define GL_R16                            0x822A
#define GL_RG8                            0x822B
#define GL_RG16                           0x822C
#define GL_R16F                           0x822D
#define GL_R32F                           0x822E
#define GL_RG16F                          0x822F
#define GL_RG32F                          0x8230
#define GL_R8I                            0x8231
#define GL_R8UI                           0x8232
#define GL_R16I                           0x8233
#define GL_R16UI                          0x8234
#define GL_R32I                           0x8235
#define GL_R32UI                          0x8236
#define GL_RG8I                           0x8237
#define GL_RG8UI                          0x8238
#define GL_RG16I                          0x8239
#define GL_RG16UI                         0x823A
#define GL_RG32I                          0x823B
#define GL_RG32UI                         0x823C
#define GL_RGBA32F                        0x8814
#define GL_VERTEX_ARRAY                   0x8074
#define GL_FRAGMENT_SHADER                0x8B30
#define GL_VERTEX_SHADER                  0x8B31
#define GL_COMPILE_STATUS                 0x8B81
#define GL_LINK_STATUS                    0x8B82
#define GL_INFO_LOG_LENGTH                0x8B84
#define GL_GEOMETRY_SHADER                0x8DD9
#define GL_BACK_LEFT                      0x0402

#define GL_TEXTURE0                       0x84C0
#define GL_TEXTURE1                       0x84C1
#define GL_TEXTURE2                       0x84C2
#define GL_TEXTURE3                       0x84C3
#define GL_TEXTURE4                       0x84C4
#define GL_TEXTURE5                       0x84C5
#define GL_TEXTURE6                       0x84C6
#define GL_TEXTURE7                       0x84C7
#define GL_TEXTURE8                       0x84C8
#define GL_TEXTURE9                       0x84C9
#define GL_TEXTURE10                      0x84CA
#define GL_TEXTURE11                      0x84CB
#define GL_TEXTURE12                      0x84CC
#define GL_TEXTURE13                      0x84CD
#define GL_TEXTURE14                      0x84CE
#define GL_TEXTURE15                      0x84CF
#define GL_TEXTURE16                      0x84D0
#define GL_TEXTURE17                      0x84D1
#define GL_TEXTURE18                      0x84D2
#define GL_TEXTURE19                      0x84D3
#define GL_TEXTURE20                      0x84D4
#define GL_TEXTURE21                      0x84D5
#define GL_TEXTURE22                      0x84D6
#define GL_TEXTURE23                      0x84D7
#define GL_TEXTURE24                      0x84D8
#define GL_TEXTURE25                      0x84D9
#define GL_TEXTURE26                      0x84DA
#define GL_TEXTURE27                      0x84DB
#define GL_TEXTURE28                      0x84DC
#define GL_TEXTURE29                      0x84DD
#define GL_TEXTURE30                      0x84DE
#define GL_TEXTURE31                      0x84DF

#define GL_FRAMEBUFFER                    0x8D40
#define GL_READ_FRAMEBUFFER               0x8CA8
#define GL_DRAW_FRAMEBUFFER               0x8CA9
#define GL_DEPTH_ATTACHMENT               0x8D00
#define GL_STENCIL_ATTACHMENT             0x8D20
#define GL_MAX_COLOR_ATTACHMENTS          0x8CDF
#define GL_COLOR_ATTACHMENT0              0x8CE0
#define GL_COLOR_ATTACHMENT1              0x8CE1
#define GL_COLOR_ATTACHMENT2              0x8CE2
#define GL_COLOR_ATTACHMENT3              0x8CE3
#define GL_COLOR_ATTACHMENT4              0x8CE4
#define GL_COLOR_ATTACHMENT5              0x8CE5
#define GL_COLOR_ATTACHMENT6              0x8CE6
#define GL_COLOR_ATTACHMENT7              0x8CE7
#define GL_COLOR_ATTACHMENT8              0x8CE8
#define GL_COLOR_ATTACHMENT9              0x8CE9
#define GL_COLOR_ATTACHMENT10             0x8CEA
#define GL_COLOR_ATTACHMENT11             0x8CEB
#define GL_COLOR_ATTACHMENT12             0x8CEC
#define GL_COLOR_ATTACHMENT13             0x8CED
#define GL_COLOR_ATTACHMENT14             0x8CEE
#define GL_COLOR_ATTACHMENT15             0x8CEF
#define GL_COLOR_ATTACHMENT16             0x8CF0
#define GL_COLOR_ATTACHMENT17             0x8CF1
#define GL_COLOR_ATTACHMENT18             0x8CF2
#define GL_COLOR_ATTACHMENT19             0x8CF3
#define GL_COLOR_ATTACHMENT20             0x8CF4
#define GL_COLOR_ATTACHMENT21             0x8CF5
#define GL_COLOR_ATTACHMENT22             0x8CF6
#define GL_COLOR_ATTACHMENT23             0x8CF7
#define GL_COLOR_ATTACHMENT24             0x8CF8
#define GL_COLOR_ATTACHMENT25             0x8CF9
#define GL_COLOR_ATTACHMENT26             0x8CFA
#define GL_COLOR_ATTACHMENT27             0x8CFB
#define GL_COLOR_ATTACHMENT28             0x8CFC
#define GL_COLOR_ATTACHMENT29             0x8CFD
#define GL_COLOR_ATTACHMENT30             0x8CFE
#define GL_COLOR_ATTACHMENT31             0x8CFF

#define GL_READ_ONLY                      0x88B8
#define GL_WRITE_ONLY                     0x88B9
#define GL_READ_WRITE                     0x88BA

#define GL_ARRAY_BUFFER                   0x8892
#define GL_ELEMENT_ARRAY_BUFFER           0x8893
#define GL_STREAM_DRAW                    0x88E0
#define GL_STREAM_READ                    0x88E1
#define GL_STREAM_COPY                    0x88E2
#define GL_STATIC_DRAW                    0x88E4
#define GL_STATIC_READ                    0x88E5
#define GL_STATIC_COPY                    0x88E6
#define GL_DYNAMIC_DRAW                   0x88E8
#define GL_DYNAMIC_READ                   0x88E9
#define GL_DYNAMIC_COPY                   0x88EA

// NOTE: Query objects
#define GL_QUERY_RESULT                   0x8866
#define GL_PRIMITIVES_GENERATED           0x8C87

// NOTE: ARB_debug_output
#define GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB                      0x8242

#define GL_MAX_DEBUG_MESSAGE_LENGTH_ARB                      0x9143
#define GL_MAX_DEBUG_LOGGED_MESSAGES_ARB                     0x9144
#define GL_DEBUG_LOGGED_MESSAGES_ARB                         0x9145
#define GL_DEBUG_NEXT_LOGGED_MESSAGE_LENGTH_ARB              0x8243

#define GL_DEBUG_CALLBACK_FUNCTION_ARB                       0x8244
#define GL_DEBUG_CALLBACK_USER_PARAM_ARB                     0x8245

#define GL_DEBUG_SOURCE_API_ARB                              0x8246
#define GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB                    0x8247
#define GL_DEBUG_SOURCE_SHADER_COMPILER_ARB                  0x8248
#define GL_DEBUG_SOURCE_THIRD_PARTY_ARB                      0x8249
#define GL_DEBUG_SOURCE_APPLICATION_ARB                      0x824A
#define GL_DEBUG_SOURCE_OTHER_ARB                            0x824B

#define GL_DEBUG_TYPE_ERROR_ARB                              0x824C
#define GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB                0x824D
#define GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB                 0x824E
#define GL_DEBUG_TYPE_PORTABILITY_ARB                        0x824F
#define GL_DEBUG_TYPE_PERFORMANCE_ARB                        0x8250
#define GL_DEBUG_TYPE_OTHER_ARB                              0x8251

#define GL_DEBUG_SEVERITY_HIGH_ARB                           0x9146
#define GL_DEBUG_SEVERITY_MEDIUM_ARB                         0x9147
#define GL_DEBUG_SEVERITY_LOW_ARB                            0x9148

#define GL_MACRO(proc, ret, ...)                \
	typedef ret APIENTRY _##proc(__VA_ARGS__); \
	global_persist _##proc * proc;

typedef void (APIENTRY *GLDEBUGPROCARB)(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const char* message, const void* userParam);

#endif // #ifndef GL_MACRO

GL_MACRO(glGetError, GLenum, void);
GL_MACRO(glTexParameteri, void, GLenum target, GLenum pname, GLint param);
GL_MACRO(glTexImage2D, void, GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid * data);
GL_MACRO(glTexImage3D, void, GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid * data);
GL_MACRO(glTexBuffer, void, GLenum target, GLenum internalformat, GLuint buffer);
GL_MACRO(glTexSubImage3D, void, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid * pixels);
GL_MACRO(glActiveTexture, void, GLenum texture);
GL_MACRO(glBindTexture, void, GLenum target, GLuint texture);
GL_MACRO(glGenTextures,void, GLsizei n, GLuint * textures);
GL_MACRO(glGenVertexArrays, void, GLsizei n, GLuint *arrays);
GL_MACRO(glGenBuffers, void, GLsizei n, GLuint * buffers);
GL_MACRO(glBufferData, void, GLenum target, GLsizeiptr size, const GLvoid * data, GLenum usage);
GL_MACRO(glBufferSubData, void, GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid * data);
GL_MACRO(glDeleteBuffers, void, GLsizei n, const GLuint * buffers);
GL_MACRO(glDeleteVertexArrays, void, GLsizei n, const GLuint *arrays);
GL_MACRO(glBufferStorage, void, GLenum target​, GLsizeiptr size​, const GLvoid * data​, GLbitfield flags​);
GL_MACRO(glMapBuffer, void *, GLenum target, GLenum access);
GL_MACRO(glUnmapBuffer, GLboolean, GLenum target);
GL_MACRO(glBindBuffer, void, GLenum target, GLuint buffer);
GL_MACRO(glBindVertexArray, void, GLuint array);
GL_MACRO(glViewport, void, GLint x, GLint y, GLsizei width, GLsizei height);
GL_MACRO(glDepthFunc, void, GLenum func);
GL_MACRO(glBlendFunc, void, GLenum sfactor, GLenum dfactor);
GL_MACRO(glColor4f, void, GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
GL_MACRO(glEnable, void, GLenum cap);
GL_MACRO(glDisable, void, GLenum cap);
GL_MACRO(glClear, void, GLbitfield mask);
GL_MACRO(glClearColor, void, GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
GL_MACRO(glPointSize, void, GLfloat size);
GL_MACRO(glEnableClientState, void, GLenum cap);
GL_MACRO(glDisableClientState, void, GLenum cap);
GL_MACRO(glDrawArrays, void, GLenum mode, GLint first, GLsizei count);
GL_MACRO(glCreateShader, GLuint, GLenum shaderType);
GL_MACRO(glShaderSource, void, GLuint shader, GLsizei count, const GLchar **string, const GLint *length);
GL_MACRO(glGetShaderInfoLog, void, GLuint shader, GLsizei maxLength, GLsizei *length, GLchar *infoLog);
GL_MACRO(glGetShaderiv, void, GLuint shader, GLenum pname, GLint *params);
GL_MACRO(glAttachShader, void, GLuint program, GLuint shader);
GL_MACRO(glCompileShader, void, GLuint shader);
GL_MACRO(glCreateProgram, GLuint, void);
GL_MACRO(glLinkProgram, void, GLuint program);
GL_MACRO(glUseProgram, void, GLuint program);
GL_MACRO(glGetUniformLocation, GLint, GLuint program, const GLchar *name);
GL_MACRO(glGetAttribLocation, GLint, GLuint program, const GLchar *name);
GL_MACRO(glBindAttribLocation, void, GLuint program, GLuint index, const GLchar *name);
GL_MACRO(glEnableVertexAttribArray, void, GLuint index);
GL_MACRO(glDisableVertexAttribArray, void, GLuint index);
GL_MACRO(glVertexAttribPointer, void, GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid * pointer);
GL_MACRO(glVertexAttribIPointer, void, GLuint index, GLint size, GLenum type, GLsizei stride, const void * pointer);
GL_MACRO(glGetProgramiv, void, GLuint program, GLenum pname, GLint *params);
GL_MACRO(glGetProgramInfoLog, void, GLuint program, GLsizei maxLength, GLsizei *length, GLchar *infoLog);
GL_MACRO(glFlush, void, void);
GL_MACRO(glFinish, void, void);
GL_MACRO(glUniform1i, void, GLint location, GLint v0);
GL_MACRO(glUniform1f, void, GLint location, GLfloat v0);
GL_MACRO(glUniform2i, void, GLint location, GLint v0, GLint v1);
GL_MACRO(glUniform2f, void, GLint location, GLfloat v0, GLfloat v1);
GL_MACRO(glUniform3f, void, GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
GL_MACRO(glUniform4f, void, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
GL_MACRO(glUniform2fv, void, GLint location, GLsizei count, const GLfloat *value);
GL_MACRO(glUniform3fv, void, GLint location, GLsizei count, const GLfloat *value);
GL_MACRO(glUniform4fv, void, GLint location, GLsizei count, const GLfloat *value);
GL_MACRO(glUniformMatrix4fv, void, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
GL_MACRO(glBindFragDataLocation, void, GLuint program, GLuint colorNumber, const char * name);
GL_MACRO(glDrawBuffers, void, GLsizei n, const GLenum *bufs);
GL_MACRO(glGenFramebuffers, void, GLsizei n, GLuint *ids);
GL_MACRO(glDeleteFramebuffers, void, GLsizei n, GLuint *framebuffers);
GL_MACRO(glBindFramebuffer, void, GLenum target, GLuint framebuffer);
GL_MACRO(glFramebufferTexture2D, void, GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
GL_MACRO(glFramebufferTextureLayer, void, GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer);
GL_MACRO(glBlitFramebuffer, void, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);

GL_MACRO(glGenQueries, void, GLsizei n, GLuint * ids);
GL_MACRO(glDeleteQueries, void, GLsizei n, const GLuint * ids);
GL_MACRO(glBeginQuery, void, GLenum target, GLuint id);
GL_MACRO(glEndQuery, void, GLenum target);
GL_MACRO(glGetQueryObjectuiv, void, GLuint id, GLenum pname, GLuint * params);

GL_MACRO(glDebugMessageControlARB, void, GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint* ids, GLboolean enabled);
GL_MACRO(glDebugMessageInsertARB, void, GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const char* buf);
GL_MACRO(glDebugMessageCallbackARB, void, GLDEBUGPROCARB callback, const void *userparam);
GL_MACRO(glGetDebugMessageLogARB, GLuint, GLuint count, GLsizei bufSize, GLenum* sources, GLenum* types, GLuint* ids, GLenum* severities, GLsizei* lengths, GLchar* messageLog);
GL_MACRO(glGetPointerv, void, GLenum pname, void** params);

#undef GL_MACRO