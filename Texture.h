#ifndef TEXTURE_CLASS_H
#define TEXTURE_CLASS_H

#ifndef __EMSCRIPTEN__
#include<glad/glad.h>
#endif
#ifdef __EMSCRIPTEN__
#include<GLES3/gl3.h>
#endif
#include<stb/stb_image.h>

#include"shaders/shaderClass/shaderClass.h"

class Texture
{
public:
	GLuint ID;
	const char* type;
	GLuint unit;

	Texture(const char* image, const char* texType, GLuint slot, GLenum format, GLenum pixelType);

	// Assigns a texture unit to a texture
	void texUnit(Shader& shader, const char* uniform, GLuint unit);
	// Binds a texture
	void Bind();
	// Unbinds a texture
	void Unbind();
	// Deletes a texture
	void Delete();
};
#endif