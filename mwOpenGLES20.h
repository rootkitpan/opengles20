#ifndef _MW_OPENGLES20_H_
#define _MW_OPENGLES20_H_

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>


#include "Graphic/Mw/doth/compile.h"
#include "Graphic/Mw/doth/gdhard.h"


typedef struct _MWOPENGLES20_NATIVE_INFO {
	// IN
	// windows: HDC
	EGLNativeDisplayType nativeDisplay;
	
	// IN
	// windows: HWND
	EGLNativeWindowType nativeWindow;
	
} MWOPENGLES20_NATIVE_INFO;


typedef struct _MWOPENGLES20_INFO {
	MWOPENGLES20_NATIVE_INFO native;
	
	EGLDisplay display;
	EGLSurface surface;
	
	GLuint vertexShader;
	GLuint fragmentShader;
	GLuint shaderProgram;
} MWOPENGLES20_INFO;


#define MW_FB_SIZE	(DOTS_X * DOTS_Y * 3)

typedef struct _MWOPENGLES20_WINDOW_INFO {
	int no;
	int depth;
	int x;
	int y;
	int dx;
	int dy;
	unsigned char bkColor[4];		// R G B and Alpha

	GLubyte buffer[MW_FB_SIZE];
} MWOPENGLES20_WINDOW_INFO;

extern char gVertexShaderSource[];
extern char gFragmentShaderSource[];

int InitEGLSettings(MWOPENGLES20_NATIVE_INFO *native);

int CompileShader(GLenum shaderType, char* source);
int LinkProgram(void);
int GenerateProgram(void);

int CreateWindowRect(MWOPENGLES20_WINDOW_INFO *info);
int CreateWindowFB(MWOPENGLES20_WINDOW_INFO *info);

int Draw(void);


#endif /* _MW_OPENGLES20_H_ */
