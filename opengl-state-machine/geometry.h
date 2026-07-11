#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <cstddef>

class Geometry
{
public:
    Geometry(const float* vertices, std::size_t floatCount);
    ~Geometry();

    Geometry(const Geometry&) = delete;
    Geometry& operator=(const Geometry&) = delete;
    Geometry(Geometry&&) = delete;
    Geometry& operator=(Geometry&&) = delete;

    void bind() const;
    void drawBound() const;
    void draw() const;

private:
    unsigned int vao_ = 0;
    unsigned int vbo_ = 0;
    int vertexCount_ = 0;
};

#endif
