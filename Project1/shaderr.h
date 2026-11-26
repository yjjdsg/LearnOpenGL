#ifndef SHADER_H
#define SHADER_H

// Shader class declaration only.
// The implementation lives in shaderr.cpp, which is the ONLY translation unit
// that gets compiled for shader code. Every other file just includes this
// header, so there is exactly one definition of each function at link time.

#include <glad/glad.h>

#include <string>
#include <glm/glm.hpp>

class Shader
{
public:
    // program id, public on purpose (mesh.cpp reads shader.ID)
    unsigned int ID = 0;

    // reads vertexPath / fragmentPath from disk, compiles and links them
    Shader(const char* vertexPath, const char* fragmentPath);

    // activate the shader
    void use() const;

    // utility uniform functions
    void setBool(const std::string& name, bool value) const;
    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
    void setVec3(const std::string& name, float x, float y, float z) const;
    void setVec3(const std::string& name, const glm::vec3& value) const;
    void setVec4(const std::string& name, float x, float y, float z, float w) const;
    void setMat4(const std::string& name, const glm::mat4& value) const;

    // true when the shader compiled and linked without errors
    bool isValid() const { return ID != 0; }

    // non-copyable: it owns a GL program object
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    ~Shader();

private:
    // utility function for checking shader compilation/linking errors
    bool checkCompileErrors(unsigned int shader, const std::string& type) const;
};

#endif // SHADER_H
