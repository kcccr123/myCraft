#ifndef EBO_CLASS_H
#define EBO_CLASS_H

#ifndef __EMSCRIPTEN__
#include<glad/glad.h>
#endif
#ifdef __EMSCRIPTEN__
#include<GLES3/gl3.h>
#endif
#include<vector>

class EBO
{
public:
	// ID reference of Elements Buffer Object
	GLuint ID;
	// Constructor that generates a Elements Buffer Object and links it to indices
	EBO(std::vector<GLuint>& indices);

	// Binds the EBO
	void Bind();
	// Unbinds the EBO
	void Unbind();
	// Deletes the EBO
	void Delete();

	void UpdateData(GLuint* newIndices, GLsizeiptr size);
};

#endif