#ifndef OBJ_LOADER_H
#define OBJ_LOADER_H

#include <glm/glm.hpp>
#include <string>
#include <vector>

struct ObjVertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
    bool hasUV;
};

struct ObjMesh
{
    // Three consecutive vertices per triangle.
    std::vector<ObjVertex> vertices;
};

// Throws std::runtime_error with a file and line number on invalid geometry.
// The optional default smoothing only applies before the first OBJ s record.
ObjMesh loadObj(const std::string& path, bool smoothByDefault = false);

#endif
