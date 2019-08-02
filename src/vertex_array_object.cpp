#define GLRT_API_EXPORT
#include "vertex_array_object.h"

// ---------------------------------------------------------------------------------------------------------------------
// PUBLIC methods
// ---------------------------------------------------------------------------------------------------------------------

VertexArrayObject::VertexArrayObject() {
    initialize();
}

VertexArrayObject::VertexArrayObject(VertexArrayObject &&vao) noexcept {
    this->operator=(std::move(vao));
}

VertexArrayObject &VertexArrayObject::operator=(VertexArrayObject &&vao) noexcept {
    vaoId = vao.vaoId;
    vboId = vao.vboId;
    iboId = vao.iboId;
    iboSize = vao.iboSize;
    vao.vaoId = 0;
    vao.vboId = 0;
    vao.iboId = 0;
    vao.iboSize = 0;
    return *this;
}

VertexArrayObject::~VertexArrayObject() {
    if (vboId != 0) {
        glDeleteBuffers(1, &vboId);
    }

    if (iboId != 0) {
        glDeleteBuffers(1, &iboId);
    }

    if (vaoId != 0) {
        glDeleteVertexArrays(1, &vaoId);
    }
}

VertexArrayObject VertexArrayObject::initAsSquare() {
    static float positions[] = {
        -1.0f, -1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f,
         1.0f, -1.0f, 0.0f,
         1.0f,  1.0f, 0.0f
    };

    static float normals[] = {
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f
    };

    static float uvs[] = {
        0.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 0.0f,
        1.0f, 1.0f
    };

    static uint32_t indices[] = {
        0, 1, 3, 0, 3, 2
    };

    VertexArrayObject vao;
    vao.allocVertexData(sizeof(float) * (12 + 12 + 8));

    GLintptr offset = 0;
    vao.setVertexData(offset, sizeof(float) * 12, positions);
    vao.setVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (const void*)offset);
    offset += sizeof(float) * 12;
    vao.setVertexData(offset, sizeof(float) * 12, normals);
    vao.setVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (const void*)offset);
    offset += sizeof(float) * 12;
    vao.setVertexData(offset, sizeof(float) * 8, uvs);
    vao.setVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (const void*)offset);

    vao.allocElementData(sizeof(uint32_t) * 6);
    vao.setElementData(0, sizeof(uint32_t) * 6, indices);

    return std::move(vao);
}

void VertexArrayObject::draw(GLenum mode) {
    bind();
    glDrawElements(mode, (GLsizei)iboSize, GL_UNSIGNED_INT, 0);
    unbind();
}

void VertexArrayObject::allocVertexData(GLsizeiptr size, GLenum usage) {
    if (vaoId == 0 || vboId == 0) {
        FatalError("VAO is not properly initialized!");
    }

    glBindVertexArray(vaoId);
    glBindBuffer(GL_ARRAY_BUFFER, vboId);
    glBufferData(GL_ARRAY_BUFFER, size, nullptr, usage);
}

void VertexArrayObject::allocElementData(GLsizeiptr size, GLenum usage) {
    if (vaoId == 0 || iboId == 0) {
        FatalError("VAO is not properly initialized!");
    }

    glBindVertexArray(vaoId);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboId);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, nullptr, usage);
}

void VertexArrayObject::setVertexData(GLintptr offset, GLsizeiptr size, const void *data) {
    if (vaoId == 0 || vboId == 0) {
        FatalError("VAO is not properly initialized!");
    }

    glBindVertexArray(vaoId);
    glBindBuffer(GL_ARRAY_BUFFER, vboId);
    glBufferSubData(GL_ARRAY_BUFFER, offset, size, data);
    glBindVertexArray(0);
}

void VertexArrayObject::setElementData(GLintptr offset, GLsizeiptr size, const void *data) {
    if (vaoId == 0 || iboId == 0) {
        FatalError("VAO is not properly initialized!");
    }

    glBindVertexArray(vaoId);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboId);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset, size, data);
    glBindVertexArray(0);

    iboSize = size / sizeof(uint32_t);
}

void VertexArrayObject::setVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized,
                                               GLsizei stride, const void *pointer) {
    if (vaoId == 0 || iboId == 0) {
        FatalError("VAO is not properly initialized!");
    }

    glBindVertexArray(vaoId);
    glBindBuffer(GL_ARRAY_BUFFER, vboId);

    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, size, type, normalized, stride, pointer);

    glBindVertexArray(0);
}

// ---------------------------------------------------------------------------------------------------------------------
// PRIVATE methods
// ---------------------------------------------------------------------------------------------------------------------

void VertexArrayObject::initialize() {
    glGenVertexArrays(1, &vaoId);
    glBindVertexArray(vaoId);

    glGenBuffers(1, &vboId);
    glBindBuffer(GL_ARRAY_BUFFER, vboId);

    glGenBuffers(1, &iboId);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboId);

    glBindVertexArray(0);
}
