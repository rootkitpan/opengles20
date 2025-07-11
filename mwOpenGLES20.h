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
	
} MWOPENGLES20_NATIVE_INFO, *PMWOPENGLES20_NATIVE_INFO;


typedef struct _MWOPENGLES20_SHADER_INFO {
	GLuint vertexShader;		// 0 not valid
	GLuint fragmentShader;		// 0 not valid
	GLuint shaderProgram;		// 0 not valid
	
	// attribute
	GLint al_position;			// -1 not valid, usually 0
	GLint al_texCoord;			// -1 not valid, usually 1
	// uniform
	GLint ul_texture;			// -1 not valid, usually 0
} MWOPENGLES20_SHADER_INFO, *PMWOPENGLES20_SHADER_INFO;


typedef struct _MWOPENGLES20_INFO {
	EGLDisplay display;
	EGLSurface surface;
} MWOPENGLES20_INFO;


#define MW_FB_SIZE	(DOTS_X * DOTS_Y * 3)

typedef struct _MWOPENGLES20_WINDOW_INFO {
	int isOpened;
	int no;
	int depth;
	int x;
	int y;
	int dx;
	int dy;
	unsigned char bkColor[4];		// R G B and Alpha

	// OpenGL ES stuff
	GLuint fbo;						// 0 not valid, starts from 1
	GLuint fboTexture;				// 0 not valid, starts from 1
	
	
	
	//GLubyte buffer[MW_FB_SIZE];
} MWOPENGLES20_WINDOW_INFO;



extern MWOPENGLES20_NATIVE_INFO gNativeInfo;
extern MWOPENGLES20_INFO gInfo;
extern MWOPENGLES20_SHADER_INFO gShaderInfo;
extern MWOPENGLES20_WINDOW_INFO gWinInfo[MAX_WINDOW];


int InitEGLSettings(MWOPENGLES20_NATIVE_INFO *native);

int CompileShader(GLenum shaderType, char* source);
int LinkProgram(void);
int GenerateProgram(void);

int CreateWindowFB(MWOPENGLES20_WINDOW_INFO *info);
int ReflectTexture2DefaultFB(int n);
int ReflectAllTexture2DefaultFB(void);


#endif /* _MW_OPENGLES20_H_ */
