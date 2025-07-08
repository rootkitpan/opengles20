#include <stdio.h>
#include <stdlib.h>

#include "myGLES.h"
#include "native.h"


#define INDEX_myVertex	(0)

MY_APP_INFO gAppInfo;


int InitEGLSettings(PMY_APP_INFO appInfo)
{
	EGLDisplay display = NULL;
	EGLBoolean bRet = EGL_FALSE;
	EGLint majorVersion = 0;
	EGLint minorVersion = 0;
	EGLint configAttributes[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_NONE
	};
	EGLint numConfigs = 0;
	EGLConfig config = NULL;
	EGLSurface surface = NULL;
	EGLContext context = NULL;
	EGLint contextAttributes[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};

	display = eglGetDisplay(appInfo->nativeDisplayType);
	if (display == EGL_NO_DISPLAY) {
		return -1;
	}

	bRet = eglInitialize(display, &majorVersion, &minorVersion);
	if (bRet != EGL_TRUE) {
		return -2;
	}

	bRet = eglBindAPI(EGL_OPENGL_ES_API);
	if (bRet != EGL_TRUE) {
		return -3;
	}

	bRet = eglChooseConfig(display, configAttributes, &config, 1, &numConfigs);
	if (bRet != EGL_TRUE) {
		return -4;
	}

	surface = eglCreateWindowSurface(display, config, appInfo->nativeWindowType, NULL);
	if (surface == EGL_NO_SURFACE) {
		return -5;
	}

	context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttributes);
	if (context == EGL_NO_CONTEXT) {
		return -6;
	}

	bRet = eglMakeCurrent(display, surface, surface, context);
	if (bRet != EGL_TRUE) {
		return -7;
	}


	appInfo->display = display;
	appInfo->surface = surface;

	return 0;
}



static int CompileShader(GLenum shaderType, PMY_APP_INFO appInfo)
{
	GLint isShaderCompiled = 0;
	GLuint shader = 0;
	char* source = NULL;

	shader = glCreateShader(shaderType);
	if (shader == 0) {
		return -1;
	}

	if (shaderType == GL_VERTEX_SHADER) {
		source = appInfo->VertexShaderSource;
	}
	else if (shaderType == GL_FRAGMENT_SHADER) {
		source = appInfo->FragmentShaderSource;
	}
	else {
		return -2;
	}
	glShaderSource(shader, 1, &source, NULL);

	glCompileShader(shader);

	glGetShaderiv(shader, GL_COMPILE_STATUS, &isShaderCompiled);
	if (!isShaderCompiled) {
		int infoLogLength = 0;
		int charactersWritten = 0;
		char log[128];

		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);
		if (infoLogLength >= 128) {
			infoLogLength = 128;
		}

		glGetShaderInfoLog(shader, infoLogLength, &charactersWritten, log);

		return -3;
	}

	if (shaderType == GL_VERTEX_SHADER) {
		appInfo->vertexShader = shader;
	} else {
		appInfo->fragmentShader = shader;
	}

	return 0;
}

static int LinkProgram(PMY_APP_INFO appInfo)
{
	GLuint shaderProgram = 0;

	shaderProgram = glCreateProgram();
	if (shaderProgram == 0) {
		return -1;
	}

	glAttachShader(shaderProgram, appInfo->vertexShader);
	glAttachShader(shaderProgram, appInfo->fragmentShader);

	glBindAttribLocation(shaderProgram, INDEX_myVertex, "myVertex");

	glLinkProgram(shaderProgram);

	glDeleteShader(appInfo->vertexShader);
	appInfo->vertexShader = 0;
	glDeleteShader(appInfo->fragmentShader);
	appInfo->fragmentShader = 0;

	GLint isLinked;
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &isLinked);
	if (!isLinked) {

	}

	appInfo->shaderProgram = shaderProgram;

	return 0;
}

static int GenerateProgramBin(PMY_APP_INFO appInfo)
{
	GLint length;
	GLenum binaryFormat;

	if (appInfo->generateProgramBin == 0) {
		return 0;
	}

	//char* str = glGetString(GL_EXTENSIONS);
	//printf("%s\n", str);

	glGetProgramiv(appInfo->shaderProgram, GL_PROGRAM_BINARY_LENGTH_OES, &length);
	void* binary = malloc(length);
	if (binary) {
		memset(binary, 0, length);
	}
	else {
		return -1;
	}

	//glGetProgramBinaryOES(shaderProgram, length, NULL, &binaryFormat, binary);

	PFNGLGETPROGRAMBINARYOESPROC glGetProgramBinaryOES = (PFNGLGETPROGRAMBINARYOESPROC)eglGetProcAddress("glGetProgramBinaryOES");
	if (glGetProgramBinaryOES) {
		glGetProgramBinaryOES(appInfo->shaderProgram, length, NULL, &binaryFormat, binary);
		if (glGetError() == GL_NO_ERROR) {
			writeToFile(appInfo->programBinFilepath, binary, length);
		}
	}

	free(binary);

	return 0;
}

int GenerateProgram(PMY_APP_INFO appInfo)
{
	if (appInfo->useCompiledShaderProgramBin == 1) {

	}
	else {
		CompileShader(GL_VERTEX_SHADER, appInfo);
		CompileShader(GL_FRAGMENT_SHADER, appInfo);
		LinkProgram(appInfo);

		GenerateProgramBin(appInfo);
	}

	return 0;
}

int Draw2(PMY_APP_INFO appInfo)
{
	int i = 0;
	GLenum error = 0;
	GLuint vertexBuffer = 0;
	static GLfloat vertexData[9] = {
		-0.5f, -0.5f, 0.0f,		// Bottom Left
		0.5f, -0.5f, 0.0f,		// Bottom Right
		0.0f, 0.5f, 0.0f,		// Top Middle
	};


	for (i = 0; i < 9; i++) {
		vertexData[i] += (GLfloat)0.01;
	}

	glGenBuffers(1, &vertexBuffer);
	error = glGetError();
	if (error != GL_NO_ERROR) {
		return -1;
	}

	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	error = glGetError();
	if (error != GL_NO_ERROR) {
		return -1;
	}

	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_DYNAMIC_DRAW);
	error = glGetError();
	if (error != GL_NO_ERROR) {
		return -1;
	}

	glUseProgram(appInfo->shaderProgram);
	error = glGetError();
	if (error != GL_NO_ERROR) {
		return -1;
	}

	eglSwapInterval(gAppInfo.display, 1);


	
	glClearColor(0.00f, 0.70f, 0.67f, 1.0f);
	error = glGetError();
	if (error != GL_NO_ERROR) {
		return -1;
	}

	glClear(GL_COLOR_BUFFER_BIT);
	error = glGetError();
	if (error != GL_NO_ERROR) {
		return -1;
	}

	glEnableVertexAttribArray(INDEX_myVertex);
	error = glGetError();
	if (error != GL_NO_ERROR) {
		return -1;
	}

	glVertexAttribPointer(INDEX_myVertex, 3, GL_FLOAT, GL_FALSE, 0, 0);
	error = glGetError();
	if (error != GL_NO_ERROR) {
		return -1;
	}

	glDrawArrays(GL_TRIANGLES, 0, 3);
	error = glGetError();
	if (error != GL_NO_ERROR) {
		return -1;
	}

	eglSwapBuffers(appInfo->display, appInfo->surface);
	error = glGetError();
	if (error != GL_NO_ERROR) {
		return -1;
	}

	return 0;
}


unsigned char gPixelData[640 * 480 * 3];



int Draw(PMY_APP_INFO appInfo)
{
	GLenum error;
	GLfloat vertexData[] = {
		// Position (x, y)    Texture Coordinates (u, v)
		-1.0f, -1.0f,        0.0f, 1.0f,  // Bottom left
		 1.0f, -1.0f,        1.0f, 1.0f,  // Bottom right
		-1.0f,  1.0f,        0.0f, 0.0f,  // Top left
		 1.0f,  1.0f,        1.0f, 0.0f   // Top right
	};

	GLuint vbo = 0;
	glGenBuffers(1, &vbo);
	error = glGetError();
	if (error != GL_NO_ERROR) {
		return -1;
	}
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	error = glGetError();
	if (error != GL_NO_ERROR) {
		return -1;
	}
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);
	error = glGetError();
	if (error != GL_NO_ERROR) {
		return -1;
	}


	GLuint positionAttribute = 0;
	glVertexAttribPointer(positionAttribute, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)0);
	glEnableVertexAttribArray(positionAttribute);
	error = glGetError();
	if (error != GL_NO_ERROR) {
		return -1;
	}

	GLuint texCoordAttribute = 1;
	glVertexAttribPointer(texCoordAttribute, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));
	glEnableVertexAttribArray(texCoordAttribute);
	error = glGetError();
	if (error != GL_NO_ERROR) {
		return -1;
	}

	// prepare gPixelData
	for (int i = 0; i < 480 * 640 * 3; i += 3) {
		gPixelData[i] = 0;   // Red
		gPixelData[i + 1] = 0; // Green
		gPixelData[i + 2] = 0; // Blue
	}


	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 
		0,
		GL_RGB,
		480,
		640,
		0,
		GL_RGB,
		GL_UNSIGNED_BYTE,
		gPixelData);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	error = glGetError();
	if (error != GL_NO_ERROR) {
		return -1;
	}

	glUseProgram(appInfo->shaderProgram);

	GLint textureLocation = glGetUniformLocation(appInfo->shaderProgram, "u_texture");
	glUniform1i(textureLocation, 0);
	error = glGetError();
	if (error != GL_NO_ERROR) {
		return -1;
	}
	glActiveTexture(GL_TEXTURE0);


	eglSwapInterval(gAppInfo.display, 1);
	error = glGetError();
	if (error != GL_NO_ERROR) {
		return -1;
	}
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	error = glGetError();
	if (error != GL_NO_ERROR) {
		return -1;
	}
	eglSwapBuffers(appInfo->display, appInfo->surface);
	error = glGetError();
	if (error != GL_NO_ERROR) {
		return -1;
	}
	return 0;
}


/*
	the original point or origin is at the center of the screen 
	in normalized device coordinates (NDC) This means:
	(0, 0) is the center of your screen.
	Negative values, like (-1, -1), point to the bottom-left corner.
	Positive values, like (1, 1), point to the top-right corner.


	Texture coordinates, often called (u, v) known as texture space
	are typically normalized between 0.0 and 1.0 for simplicity
	(0.0, 0.0) refers to the bottom-left corner of the texture.
	(1.0, 0.0) refers to the bottom-right corner.
	(0.0, 1.0) refers to the top-left corner.
	(1.0, 1.0) refers to the top-right corner.


	In OpenGL Shader Language (GLSL), 
	varying is a keyword used to pass data between the vertex shader and fragment shader. 
	It acts as a bridge, 
	allowing values computed in the vertex shader 
	(like texture coordinates or colors) to be interpolated 
	across the surface of a primitive (triangle, quad, etc.) 
	and then used in the fragment shader.
*/


/*
	下载OpenGL ES的SDK
	属性-->VC++目录-->包含目录				添加include目录
	属性-->链接器-->常规-->附加库目录			添加lib目录
	属性-->链接器-->输入-->附加依赖项			添加具体的lib文件

	属性-->链接器-->系统-->子系统				选择窗口

	属性-->C/C++-->预处理器-->预处理器定义		GL_GLEXT_PROTOTYPES
*/
