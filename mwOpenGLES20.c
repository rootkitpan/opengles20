#include <stdio.h>
#include <stdlib.h>

#include "mwOpenGLES20.h"


MWOPENGLES20_INFO gInfo;
MWOPENGLES20_WINDOW_INFO gWinInfo[MAX_WINDOW];


static GLint ATTR_LOCATION_POSITION;
static GLint ATTR_LOCATION_TEXCOORD;

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

		gInfo.native.nativeDisplay = native->nativeDisplay;
		gInfo.native.nativeWindow = native->nativeWindow;
		gInfo.display = display;
		gInfo.surface = surface;
		ret = 0;
	} while(0);

	return ret;
}


char gVertexShaderSource[] = "									\
	attribute vec4 a_position;									\
	attribute vec2 a_texCoord;									\
	varying vec2 v_texCoord;									\
																\
	void main(void)												\
	{															\
		gl_Position = a_position;								\
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
			gInfo.vertexShader = shader;
		} else {
			gInfo.fragmentShader = shader;
		}

		ret = 0;
	} while(0);
	
	if( ret < 0 ){
		if( shader != 0 ){
			glDeleteShader(shader);
			shader = 0;
			
			if (shaderType == GL_VERTEX_SHADER) {
				gInfo.vertexShader = 0;
			} else {
				gInfo.fragmentShader = 0;
			}
		}
	}

	return ret;
}


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

		glAttachShader(shaderProgram, gInfo.vertexShader);
		error = glGetError();
		if (error != GL_NO_ERROR) {
			ret = -2;
			break;
		}
		glAttachShader(shaderProgram, gInfo.fragmentShader);
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

		gInfo.shaderProgram = shaderProgram;

		glDeleteShader(gInfo.vertexShader);
		gInfo.vertexShader = 0;
		glDeleteShader(gInfo.fragmentShader);
		gInfo.fragmentShader = 0;

		ret = 0;
	} while (0);

	if (ret < 0) {
		if (shaderProgram != 0) {
			glDeleteProgram(shaderProgram);
			shaderProgram = 0;
			gInfo.shaderProgram = 0;
		}
	}

	return ret;
}



int GenerateProgram(void)
{
	int ret = -1;
	
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

		// get location
		ATTR_LOCATION_POSITION = glGetAttribLocation(gInfo.shaderProgram, "a_position");
		if (ATTR_LOCATION_POSITION == -1) {
			ret = -4;
			break;
		}
		ATTR_LOCATION_TEXCOORD = glGetAttribLocation(gInfo.shaderProgram, "a_texCoord");
		if (ATTR_LOCATION_TEXCOORD == -1) {
			ret = -5;
			break;
		}

		ret = 0;
	} while (0);

	if (ret <= -2) {
		glDeleteShader(gInfo.fragmentShader);
		gInfo.fragmentShader = 0;
	}
	if (ret <= -3) {
		glDeleteShader(gInfo.vertexShader);
		gInfo.vertexShader = 0;
	}
	if (ret <= -4) {
		glDeleteProgram(gInfo.shaderProgram);
		gInfo.shaderProgram = 0;
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

	glEnableVertexAttribArray(ATTR_LOCATION_POSITION);
	glVertexAttribPointer(
		ATTR_LOCATION_POSITION,			// index
		2, GL_FLOAT,					// count and type for one point
		GL_FALSE,						// normalized
		2 * sizeof(GLfloat),			// stride
		(void*)0
	);
	glUseProgram(gInfo.shaderProgram);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	ret = 0;
	
	
	return ret;
}



int CreateWindowFB(MWOPENGLES20_WINDOW_INFO *info)
{
	int ret = -1;
	GLuint fbo = 0;
	GLenum error;
	GLuint fboTexture = 0;
	GLenum checkResult = 0;


	do {
		// texture - create
		glGenTextures(1, &fboTexture);
		glBindTexture(GL_TEXTURE_2D, fboTexture);
		// texture - allocate memory, no data yet
		glTexImage2D(GL_TEXTURE_2D,
			0,						// mip level, always 0
			GL_RGB,					// internalformat: how the texture will be stored internally on the GPU
			info->dx, info->dy,		// width and height
			0,						// border, always 0
			GL_RGB,					// format: the format of the source data being uploaded to the GPU
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


		// create FBO
		glGenFramebuffers(1, &fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		error = glGetError();
		if (error != GL_NO_ERROR) {
			ret = -1;
			break;
		}
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

		ret = 0;
	} while (0);
	


	// upload 2D texture data
	{
		int i;

		for (i = 0; i < MW_FB_SIZE; i += 3) {
			info->buffer[i] = info->bkColor[0];
			info->buffer[i+1] = info->bkColor[1];
			info->buffer[i+2] = info->bkColor[2];
		}

		glBindTexture(GL_TEXTURE_2D, fboTexture);
		glTexSubImage2D(
			GL_TEXTURE_2D,
			0,
			0, 0, info->dx, info->dy,
			GL_RGB,
			GL_UNSIGNED_BYTE,
			info->buffer
		);
		error = glGetError();
		if (error != GL_NO_ERROR) {
			ret = -1;
		}
	}

	// render FBO
	{
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glViewport(0, 0, info->dx, info->dy);

		glClearColor(0.2f, 0.3f, 0.5f, 1.0f); // Example: Dark blue
		glClear(GL_COLOR_BUFFER_BIT);

		// (Optional) Render additional shapes here
		// TODO
	}

	glUseProgram(gInfo.shaderProgram);

	GLint textureUniform = glGetUniformLocation(gInfo.shaderProgram, "u_texture");
	if (textureUniform == -1) {
		printf("Failed to get uniform location for u_texture!\n");
		ret = -1;
	}
	glUniform1i(textureUniform, 0);
	glActiveTexture(GL_TEXTURE0);


	// Rendering FBO Texture to Screen
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, info->dx, info->dy);


		glBindTexture(GL_TEXTURE_2D, fboTexture);

		GLfloat vertices[] = {
			// Positions   // Texture Coordinates
			-1.0f, -1.0f,   0.0f, 0.0f, // Bottom-left
			 1.0f, -1.0f,   1.0f, 0.0f, // Bottom-right
			-1.0f,  1.0f,   0.0f, 1.0f, // Top-left
			 1.0f,  1.0f,   1.0f, 1.0f  // Top-right
		};

		// Set vertex attributes and draw the quad
		glEnableVertexAttribArray(0); // Position attribute
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), vertices);

		glEnableVertexAttribArray(1); // Texture coordinates attribute
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), vertices + 2);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}
	return ret;
}



int Draw(void)
{
	GLenum error;
	GLfloat vertexData[] = {
		// Position (x, y)    Texture Coordinates (u, v)
		-1.0f, -1.0f,        0.0f, 1.0f,  // Bottom left
		 1.0f, -1.0f,        1.0f, 1.0f,  // Bottom right
		-1.0f,  1.0f,        0.0f, 0.0f,  // Top left
		 1.0f,  1.0f,        1.0f, 0.0f   // Top right
	};


	glUseProgram(gInfo.shaderProgram);

	glVertexAttribPointer(ATTR_LOCATION_POSITION, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)0);
	glEnableVertexAttribArray(ATTR_LOCATION_POSITION);
	error = glGetError();
	if (error != GL_NO_ERROR) {
		return -1;
	}

	glVertexAttribPointer(ATTR_LOCATION_TEXCOORD, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));
	glEnableVertexAttribArray(ATTR_LOCATION_TEXCOORD);
	error = glGetError();
	if (error != GL_NO_ERROR) {
		return -1;
	}


	GLint textureLocation = glGetUniformLocation(gInfo.shaderProgram, "u_texture");
	glUniform1i(textureLocation, 0);
	error = glGetError();
	if (error != GL_NO_ERROR) {
		return -1;
	}
	glActiveTexture(GL_TEXTURE0);



	eglSwapInterval(gInfo.display, 1);
	error = glGetError();
	if (error != GL_NO_ERROR) {
		return -1;
	}
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	error = glGetError();
	if (error != GL_NO_ERROR) {
		return -1;
	}
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
