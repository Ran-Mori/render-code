#include "geometry.h"

#include <glad/glad.h>

Geometry::Geometry(const float* vertices, std::size_t floatCount)
    : vertexCount_(static_cast<int>(floatCount / 2))
{
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(floatCount * sizeof(float)),
        vertices,
        GL_STATIC_DRAW
    );
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
}

Geometry::~Geometry()
{
    glDeleteVertexArrays(1, &vao_);
    glDeleteBuffers(1, &vbo_);
}

void Geometry::bind() const
{
    glBindVertexArray(vao_);
}

void Geometry::drawBound() const
{
    glDrawArrays(GL_TRIANGLES, 0, vertexCount_);
}

void Geometry::draw() const
{
    bind();
    drawBound();
}
