#ifdef _MSC_VER
#pragma once
#endif

#ifndef VERTEX_ARRAY_OBJECT_H
#define VERTEX_ARRAY_OBJECT_H

#include "api.h"
#include "common.h"

class GLRT_API VertexArrayObject {
public:
    // PUBLIC methods
    VertexArrayObject();
    VertexArrayObject(VertexArrayObject &&vao) noexcept;
    VertexArrayObject &operator=(VertexArrayObject &&vao) noexcept;
    virtual ~VertexArrayObject();

    static VertexArrayObject initAsSquare();

    void draw(GLenum mode = GL_TRIANGLES);

    inline void bind() { glBindVertexArray(vaoId); }
    inline void unbind() { glBindVertexArray(0); }

    inline GLuint getVBO() const { return vboId; }

    void allocVertexData(GLsizeiptr size, GLenum usage = GL_STATIC_DRAW);
    void allocElementData(GLsizeiptr size, GLenum usage = GL_STATIC_DRAW);
    void setVertexData(GLintptr offset, GLsizeiptr size, const void *data);
    void setElementData(GLintptr offset, GLsizeiptr size, const void *data);
    void setVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride,
                                const void *pointer);

private:
    // PRIVATE methods
    void initialize();

    // PRIVATE parameters
    GLuint vaoId = 0;
    GLuint vboId = 0;
    GLuint iboId = 0;
    size_t iboSize = 0;
};

#endif  // VERTEX_ARRAY_OBJECT_H
