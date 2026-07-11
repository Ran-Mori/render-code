#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

class Shader
{
public:
    Shader(const char* vertexPath, const char* fragmentPath)
    {
        unsigned int vertex = 0;
        unsigned int fragment = 0;

        try
        {
            vertex = compile(GL_VERTEX_SHADER, readFile(vertexPath), "VERTEX");
            fragment = compile(GL_FRAGMENT_SHADER, readFile(fragmentPath), "FRAGMENT");
            id_ = link(vertex, fragment);
        }
        catch (...)
        {
            if (vertex != 0)
                glDeleteShader(vertex);
            if (fragment != 0)
                glDeleteShader(fragment);
            throw;
        }

        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }

    ~Shader()
    {
        if (id_ != 0)
            glDeleteProgram(id_);
    }

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&&) = delete;
    Shader& operator=(Shader&&) = delete;

    void use() const
    {
        glUseProgram(id_);
    }

    unsigned int id() const
    {
        return id_;
    }

private:
    static std::string readFile(const char* path)
    {
        std::ifstream file;
        file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

        try
        {
            file.open(path);
            std::stringstream stream;
            stream << file.rdbuf();
            return stream.str();
        }
        catch (const std::ifstream::failure& error)
        {
            throw std::runtime_error(
                std::string("Failed to read shader file: ") + path + " (" + error.what() + ")"
            );
        }
    }

    static unsigned int compile(
        GLenum type,
        const std::string& source,
        const char* typeName
    )
    {
        const char* sourcePointer = source.c_str();
        const unsigned int shader = glCreateShader(type);
        glShaderSource(shader, 1, &sourcePointer, nullptr);
        glCompileShader(shader);

        int success = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char infoLog[1024];
            glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
            glDeleteShader(shader);
            throw std::runtime_error(
                std::string("Failed to compile ") + typeName + " shader:\n" + infoLog
            );
        }
        return shader;
    }

    static unsigned int link(unsigned int vertex, unsigned int fragment)
    {
        const unsigned int program = glCreateProgram();
        glAttachShader(program, vertex);
        glAttachShader(program, fragment);
        glLinkProgram(program);

        int success = 0;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success)
        {
            char infoLog[1024];
            glGetProgramInfoLog(program, sizeof(infoLog), nullptr, infoLog);
            glDeleteProgram(program);
            throw std::runtime_error(
                std::string("Failed to link shader program:\n") + infoLog
            );
        }
        return program;
    }

    unsigned int id_ = 0;
};

#endif
