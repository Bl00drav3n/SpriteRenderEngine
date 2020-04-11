#define WGL_PF(proc, ret, ...)               \
	typedef ret WINAPI _##proc(__VA_ARGS__); \
	global_persist _##proc * proc;

WGL_PF(wglGetExtensionsStringARB, const char *, HDC hDC);
WGL_PF(wglChoosePixelFormatARB, BOOL, HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);
WGL_PF(wglSwapBuffers, BOOL, HDC hdc);
WGL_PF(wglSwapInterval, BOOL, int Interval);
WGL_PF(wglCreateContextAttribsARB, HGLRC, HDC, HGLRC, const int *);

#undef WGL_PF

#define WGL_DRAW_TO_WINDOW_ARB					0x2001
#define WGL_SUPPORT_OPENGL_ARB					0x2010
#define WGL_DOUBLE_BUFFER_ARB					0x2011
#define WGL_PIXEL_TYPE_ARB						0x2013
#define WGL_COLOR_BITS_ARB						0x2014
#define WGL_SAMPLE_BUFFERS_ARB					0x2041
#define WGL_SAMPLES_ARB							0x2042
#define WGL_TYPE_RGBA_ARB						0x202B

#define WGL_CONTEXT_MAJOR_VERSION_ARB			0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB           0x2092
#define WGL_CONTEXT_PROFILE_MASK_ARB            0x9126
#define WGL_CONTEXT_FLAGS_ARB                   0x2094
#define WGL_CONTEXT_DEBUG_BIT_ARB               0x0001

#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB			0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB	0x00000002

struct extension_string
{
	char *Str;
	umm Len;
};

#define MAX_WGL_EXTENSION_STRINGS 1024
struct wgl_extensions
{
	b32 Valid;
	char *ExtensionsString;

	u32 Count;
	extension_string Extensions[MAX_WGL_EXTENSION_STRINGS];
};

global_persist wgl_extensions WGL_Extensions;

internal BOOL AddExtensionWGL(char *Str, umm Len)
{
	BOOL Result = FALSE;
	if(WGL_Extensions.Count < MAX_WGL_EXTENSION_STRINGS) {
		extension_string *Ext = WGL_Extensions.Extensions + WGL_Extensions.Count++;
		Ext->Str = Str;
		Ext->Len = Len;
		Result = TRUE;
	}

	return Result;
}

internal void LoadExtensionsWGL()
{
	if(WGL_Extensions.Valid) {
		char *Str = WGL_Extensions.ExtensionsString;
		char *At;
		for(At = Str; *At; At++) {
			if(*At == ' ') {
				AddExtensionWGL(Str, At - Str);
				Str = ++At;
			}
		}

		if(*Str) {
			AddExtensionWGL(Str, At - Str);
		}
	}
}

internal BOOL ExtensionSupported(char *Str)
{
	BOOL Result = FALSE;
	if(WGL_Extensions.Valid) {
		for(u32 i = 0; i < WGL_Extensions.Count; i++) {
			extension_string *Ext = WGL_Extensions.Extensions + i;
			char *At = Str;
			for(u32 i = 0; i < Ext->Len && *At; i++) {
				if(*At++ != Ext->Str[i]) {
					break;
				}
			}

			if(At - Str == Ext->Len && !*At) {
				Result = TRUE;
				break;
			}
		}
	}

	return Result;
}

PLATFORM_LOAD_OPENGL_FUNCTION(Win32GetProcAddressGL)
{
	void *Ptr = (void *)wglGetProcAddress(Func);
	if(Ptr == 0 || (Ptr == (void*)0x1) || (Ptr == (void*)0x2) || (Ptr == (void*)0x3) || (Ptr == (void*)-1)) {
		// TODO: Save module?
		HMODULE Module = LoadLibraryA("opengl32.dll");
		Ptr = (void *)GetProcAddress(Module, Func);
		FreeLibrary(Module);
	}

	return Ptr;
}