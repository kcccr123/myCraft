#ifndef VAO_CLASS_H
#define VAO_CLASS_H

#ifndef __EMSCRIPTEN__
#include<glad/glad.h>
#endif
#ifdef __EMSCRIPTEN__
#include<GLES3/gl3.h>
#endif
#include"VBO.h"

class VAO
{
public:
	// ID reference for the Vertex Array Object
	GLuint ID;
	// Constructor that generates a VAO ID
	VAO();

	// Links a VBO Attribute such as a position or color to the VAO
	void LinkAttrib(VBO& VBO, GLuint layout, GLuint numComponents, GLenum type, GLsizeiptr stride, void* offset);
	// Binds the VAO
	void Bind();
	// Unbinds the VAO
	void Unbind();
	// Deletes the VAO
	void Delete();
};

#endif