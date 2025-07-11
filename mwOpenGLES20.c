#include "mwOpenGLES20.h"


MWOPENGLES20_NATIVE_INFO gNativeInfo;
MWOPENGLES20_INFO gInfo;
MWOPENGLES20_SHADER_INFO gShaderInfo;
MWOPENGLES20_WINDOW_INFO gWinInfo[MAX_WINDOW];



//
int InitEGLSettings(MWOPENGLES20_NATIVE_INFO *native)
{
	EGLDisplay display = NULL;
	EGLBoolean bRet = EGL_FALSE;
	EGLint majorVersion = 0;
	EGLint minorVersion = 0;
	EGLint configAttributes[] = {
		EGL_SURFACE_TYPE,		EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE,	EGL_OPENGL_ES2_BIT,
		EGL_RED_SIZE,			8,
		EGL_GREEN_SIZE,			8,
		EGL_BLUE_SIZE,			8,
		EGL_ALPHA_SIZE,			8,
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
	int ret = 0;

	do{
		display = eglGetDisplay(native->nativeDisplay);
		if (display == EGL_NO_DISPLAY) {
			ret = -1;
			break;
		}

		bRet = eglInitialize(display, &majorVersion, &minorVersion);
		if (bRet != EGL_TRUE) {
			ret = -2;
			break;
		}

		bRet = eglChooseConfig(display, configAttributes, &config, 1, &numConfigs);
		if (bRet != EGL_TRUE) {
			ret = -3;
			break;
		}

		surface = eglCreateWindowSurface(display, config, native->nativeWindow, NULL);
		if (surface == EGL_NO_SURFACE) {
			ret = -4;
			break;
		}

		/* call eglBindAPI to tell current thread which Graphic API 
		 * will be used in this thread before creating context */
		bRet = eglBindAPI(EGL_OPENGL_ES_API);
		if (bRet != EGL_TRUE) {
			ret = -5;
			break;
		}

		context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttributes);
		if (context == EGL_NO_CONTEXT) {
			ret = -6;
			break;
		}

		bRet = eglMakeCurrent(display, surface, surface, context);
		if (bRet != EGL_TRUE) {
			ret = -7;
			break;
		}

		gNativeInfo.nativeDisplay = native->nativeDisplay;
		gNativeInfo.nativeWindow = native->nativeWindow;
		gInfo.display = display;
		gInfo.surface = surface;
		ret = 0;
	} while(0);

	return ret;
}


char gVertexShaderSource[] = "									\
	attribute vec2 a_position;									\
	attribute vec2 a_texCoord;									\
	varying vec2 v_texCoord;									\
																\
	void main(void)												\
	{															\
		gl_Position = vec4(a_position, 0.0, 0.1);				\
		v_texCoord = a_texCoord;								\
	}															\
";

char gFragmentShaderSource[] = "								\
	precision mediump float;									\
																\
	varying vec2 v_texCoord;									\
	uniform sampler2D u_texture;								\
																\
	void main (void)											\
	{															\
		gl_FragColor = texture2D(u_texture, v_texCoord);		\
	}															\
";


//
int CompileShader(GLenum shaderType, char* source)
{
	GLint isShaderCompiled = 0;
	GLuint shader = 0;
	int ret = -1;

	do {
		if(
			(shaderType != GL_VERTEX_SHADER) &&
			(shaderType != GL_FRAGMENT_SHADER)
		){
			ret = -1;
			break;
		}
		
		shader = glCreateShader(shaderType);
		if (shader == 0) {
			ret = -2;
			break;
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

			ret = -3;
			break;
		}

		if (shaderType == GL_VERTEX_SHADER) {
			gShaderInfo.vertexShader = shader;
		} else {
			gShaderInfo.fragmentShader = shader;
		}

		ret = 0;
	} while(0);
	
	if( ret < 0 ){
		if( shader != 0 ){
			glDeleteShader(shader);
			shader = 0;
			
			if (shaderType == GL_VERTEX_SHADER) {
				gShaderInfo.vertexShader = 0;
			} else {
				gShaderInfo.fragmentShader = 0;
			}
		}
	}

	return ret;
}


//
int LinkProgram(void)
{
	GLuint shaderProgram = 0;
	int ret = -1;
	GLenum error;
	GLint isLinked;

	do {
		shaderProgram = glCreateProgram();
		if (shaderProgram == 0) {
			ret = -1;
			break;
		}

		glAttachShader(shaderProgram, gShaderInfo.vertexShader);
		error = glGetError();
		if (error != GL_NO_ERROR) {
			ret = -2;
			break;
		}
		glAttachShader(shaderProgram, gShaderInfo.fragmentShader);
		error = glGetError();
		if (error != GL_NO_ERROR) {
			ret = -3;
			break;
		}

		glLinkProgram(shaderProgram);
		glGetProgramiv(shaderProgram, GL_LINK_STATUS, &isLinked);
		if (!isLinked) {
			ret = -4;
			break;
		}

		gShaderInfo.shaderProgram = shaderProgram;

		glDeleteShader(gShaderInfo.vertexShader);
		gShaderInfo.vertexShader = 0;
		glDeleteShader(gShaderInfo.fragmentShader);
		gShaderInfo.fragmentShader = 0;

		ret = 0;
	} while (0);

	if (ret < 0) {
		if (shaderProgram != 0) {
			glDeleteProgram(shaderProgram);
			shaderProgram = 0;
			gShaderInfo.shaderProgram = 0;
		}
	}

	return ret;
}


//
int GenerateProgram(void)
{
	int ret = -1;
	GLint location = -1;
	
	do {
		ret = CompileShader(GL_VERTEX_SHADER, gVertexShaderSource);
		if (ret != 0) {
			ret = -1;
			break;
		}
		ret = CompileShader(GL_FRAGMENT_SHADER, gFragmentShaderSource);
		if (ret != 0) {
			ret = -2;
			break;
		}
		ret = LinkProgram();
		if (ret != 0) {
			ret = -3;
			break;
		}

		// get location - a_position
		location = glGetAttribLocation(gShaderInfo.shaderProgram, "a_position");
		if (location == -1) {
			ret = -4;
			break;
		}
		gShaderInfo.al_position = location;
		
		// get location - a_texCoord
		location = glGetAttribLocation(gShaderInfo.shaderProgram, "a_texCoord");
		if (location == -1) {
			ret = -5;
			break;
		}
		gShaderInfo.al_texCoord = location;
		
		// get location - u_texture
		location = glGetUniformLocation(gShaderInfo.shaderProgram, "u_texture");
		if (location == -1) {
			ret = -6;
			break;
		}
		gShaderInfo.ul_texture = location;
		
		
		glUseProgram(gShaderInfo.shaderProgram);
		
		
		ret = 0;
	} while (0);

	if (ret <= -2) {
		glDeleteShader(gShaderInfo.vertexShader);
		gShaderInfo.vertexShader = 0;
	}
	if (ret <= -3) {
		glDeleteShader(gShaderInfo.fragmentShader);
		gShaderInfo.fragmentShader = 0;
	}
	if (ret <= -4) {
		glDeleteProgram(gShaderInfo.shaderProgram);
		gShaderInfo.shaderProgram = 0;
	}
	if (ret <= -5) {
		gShaderInfo.al_position = -1;
	}
	if (ret <= -6) {
		gShaderInfo.al_texCoord = -1;
	}
	
	return ret;
}


int CreateWindowRect(MWOPENGLES20_WINDOW_INFO *info)
{
	int ret = -1;
	GLuint vbo;
	
	// Vertex data for the quad
	GLfloat vertices[8] = {
		0.0f, 0.0f,			// Bottom-left corner
		0.0f, 0.0f,			// Bottom-right corner
		0.0f, 0.0f,			// Top-left corner
		0.0f, 0.0f			// Top-right corner
	};
	vertices[2] = (GLfloat)info->dx;
	vertices[5] = (GLfloat)info->dy;
	vertices[6] = (GLfloat)info->dx;
	vertices[7] = (GLfloat)info->dy;
	
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(gShaderInfo.al_position);
	glVertexAttribPointer(
		gShaderInfo.al_position,			// index
		2, GL_FLOAT,					// count and type for one point
		GL_FALSE,						// normalized
		2 * sizeof(GLfloat),			// stride
		(void*)0
	);
	glUseProgram(gShaderInfo.shaderProgram);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	ret = 0;
	
	
	return ret;
}


//
int CreateWindowFB(MWOPENGLES20_WINDOW_INFO *info)
{
	int ret = -1;
	GLuint fbo = 0;
	GLenum error;
	GLuint fboTexture = 0;
	GLenum checkResult = 0;


	do {
		// create FBO
		glGenFramebuffers(1, &fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		
		// texture - create
		glGenTextures(1, &fboTexture);
		glBindTexture(GL_TEXTURE_2D, fboTexture);
		// texture - allocate memory, no data yet
		glTexImage2D(GL_TEXTURE_2D,
			0,						// mip level, always 0
			GL_RGBA,				// internalformat: how the texture will be stored internally on the GPU
			info->dx, info->dy,		// width and height
			0,						// border, always 0
			GL_RGBA,				// format: the format of the source data being uploaded to the GPU
			GL_UNSIGNED_BYTE,		// type
			NULL					// data: memory is allocated but uninitialized
		);
		error = glGetError();
		if (error != GL_NO_ERROR) {
			ret = -1;
			break;
		}
		// texture - set parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// connect FBO to 2Dtexture we created just now
		glFramebufferTexture2D(
			GL_FRAMEBUFFER,
			GL_COLOR_ATTACHMENT0,
			GL_TEXTURE_2D,
			fboTexture,
			0
		);
		error = glGetError();
		if (error != GL_NO_ERROR) {
			ret = -1;
			break;
		}
		// check if the FBO is complete
		checkResult = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (checkResult != GL_FRAMEBUFFER_COMPLETE) {
			ret = -1;
			break;
		}

		// unbind FBO
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		info->fbo = fbo;
		info->fboTexture = fboTexture;

		ret = 0;
	} while (0);
	

	return ret;
}

int ReflectTexture2DefaultFB(int n)
{
	GLenum error;
	int ret = -1;
	MWOPENGLES20_WINDOW_INFO* winfo;

	GLfloat vertices[] = {
		// Positions   // Texture Coordinates
		-1.0f, -1.0f,   0.0f, 0.0f, // Bottom-left
		 1.0f, -1.0f,   1.0f, 0.0f, // Bottom-right
		-1.0f,  1.0f,   0.0f, 1.0f, // Top-left
		 1.0f,  1.0f,   1.0f, 1.0f  // Top-right
	};

	if (n >= MAX_WINDOW) {
		return -1;
	}

	winfo = &gWinInfo[n];
	if (!winfo->isOpened) {
		return 0;
	}

	do{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, winfo->fboTexture);
		glUniform1i(gShaderInfo.ul_texture, 0);

		// Rendering FBO Texture to Screen
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(winfo->x, winfo->y, winfo->dx, winfo->dy);

		// Set vertex attributes and draw the quad
		glVertexAttribPointer(gShaderInfo.al_position, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), vertices);
		error = glGetError();
		if (error != GL_NO_ERROR) {
			ret = -1;
			break;
		}
		glEnableVertexAttribArray(gShaderInfo.al_position);

		glVertexAttribPointer(gShaderInfo.al_texCoord, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), vertices + 2);
		glEnableVertexAttribArray(gShaderInfo.al_texCoord);
		error = glGetError();
		if (error != GL_NO_ERROR) {
			ret = -1;
			break;
		}

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		ret = 0;
	}while (0);

	return ret;
}

int ReflectAllTexture2DefaultFB(void)
{
	int i;

	for (i = 0; i < MAX_WINDOW; i++) {
		ReflectTexture2DefaultFB(i);
	}
}

int Draw(void)
{
	GLenum error;
	GLfloat quadVertices[] = {
		-1.0f, -1.0f, 0.0f, 0.0f,
		 1.0f, -1.0f, 1.0f, 0.0f,
		-1.0f,  1.0f, 0.0f, 1.0f,
		 1.0f,  1.0f, 1.0f, 1.0f
	};


	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), quadVertices);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), quadVertices + 2);
	glEnableVertexAttribArray(1);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	eglSwapBuffers(gInfo.display, gInfo.surface);
	error = glGetError();
	if (error != GL_NO_ERROR) {
		return -1;
	}
	return 0;
}



/*

glEnable(GL_BLEND);
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);



*/
