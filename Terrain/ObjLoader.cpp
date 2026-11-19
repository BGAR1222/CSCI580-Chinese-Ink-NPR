#include "ObjLoader.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <locale>
#include <map>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace
{
const std::size_t missing = static_cast<std::size_t>(-1);
const double epsilon = 1e-12;

struct Corner
{
    std::size_t position;
    std::size_t uv;
    std::size_t normal;
};

struct Face
{
    std::vector<Corner> corners;
    std::vector<std::array<std::size_t, 3>> triangles;
    glm::dvec3 areaNormal;
    long long smoothingGroup;
};

std::size_t readIndex(const std::string& token, std::size_t count)
{
    std::istringstream stream(token);
    stream.imbue(std::locale::classic());
    long long index;
    if (!(stream >> index) || stream.peek() != EOF || index == 0)
        throw std::runtime_error("invalid OBJ index '" + token + "'");

    const long long limit = static_cast<long long>(count);
    if (index > limit || index < -limit)
        throw std::runtime_error("OBJ index out of range: " + token);

    return static_cast<std::size_t>(index > 0 ? index - 1 : limit + index);
}

// Parse corner of face into zero-based position, UV, and normal indices.
// Corner format is : position index / uv index / normal index
Corner readCorner(const std::string& token, std::size_t positions,
                  std::size_t uvs, std::size_t normals)
{
    const auto first = token.find('/');
    Corner corner{readIndex(token.substr(0, first), positions), missing, missing};
    if (first == std::string::npos)
        return corner;

    const auto second = token.find('/', first + 1);
    if (second == std::string::npos)
    {
        corner.uv = readIndex(token.substr(first + 1), uvs);
        return corner;
    }

    if (second > first + 1)
        corner.uv = readIndex(token.substr(first + 1, second - first - 1), uvs);
    corner.normal = readIndex(token.substr(second + 1), normals);
    return corner;
}

double turn(const glm::dvec2& a, const glm::dvec2& b, const glm::dvec2& c)
{
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

void triangulate(Face& face, const std::vector<glm::vec3>& positions)
{
    if (face.corners.size() < 3)
        throw std::runtime_error("face requires at least three vertices");

    const glm::dvec3 origin(positions[face.corners.front().position]);
    face.areaNormal = glm::dvec3(0.0);
    for (std::size_t i = 1; i + 1 < face.corners.size(); ++i)
    {
        const glm::dvec3 a(positions[face.corners[i].position]);
        const glm::dvec3 b(positions[face.corners[i + 1].position]);
        face.areaNormal += glm::cross(a - origin, b - origin);
    }
    if (glm::dot(face.areaNormal, face.areaNormal) == 0.0)
        throw std::runtime_error("face has zero area");

    if (face.corners.size() == 3)
    {
        face.triangles.push_back({{0, 1, 2}});
        return;
    }

    // Project onto the face's dominant plane and clip ears in input winding.
    const glm::dvec3 magnitude = glm::abs(face.areaNormal);
    int axis = magnitude.x > magnitude.y ? 0 : 1;
    if (magnitude.z > magnitude[axis])
        axis = 2;
    const double winding = face.areaNormal[axis] > 0.0 ? 1.0 : -1.0;

    std::vector<glm::dvec2> points;
    std::vector<std::size_t> remaining;
    double extent = 0.0;
    for (const Corner& corner : face.corners)
    {
        const glm::dvec3 p = glm::dvec3(positions[corner.position]) - origin;
        const glm::dvec2 projected(p[(axis + 1) % 3], p[(axis + 2) % 3]);
        extent = (std::max)(extent, (std::max)(std::abs(projected.x), std::abs(projected.y)));
        remaining.push_back(points.size());
        points.push_back(projected);
    }
    for (auto& point : points)
        point /= extent;

    while (remaining.size() > 3)
    {
        bool clipped = false;
        for (std::size_t i = 0; i < remaining.size(); ++i)
        {
            const std::size_t a = remaining[(i + remaining.size() - 1) % remaining.size()];
            const std::size_t b = remaining[i];
            const std::size_t c = remaining[(i + 1) % remaining.size()];
            if (winding * turn(points[a], points[b], points[c]) <= epsilon)
                continue;

            bool containsCorner = false;
            for (std::size_t p : remaining)
            {
                if (p != a && p != b && p != c &&
                    winding * turn(points[a], points[b], points[p]) >= -epsilon &&
                    winding * turn(points[b], points[c], points[p]) >= -epsilon &&
                    winding * turn(points[c], points[a], points[p]) >= -epsilon)
                {
                    containsCorner = true;
                    break;
                }
            }
            if (containsCorner)
                continue;

            face.triangles.push_back({{a, b, c}});
            remaining.erase(remaining.begin() + i);
            clipped = true;
            break;
        }
        if (!clipped)
            throw std::runtime_error("cannot triangulate face");
    }
    const std::size_t a = remaining[0], b = remaining[1], c = remaining[2];
    if (winding * turn(points[a], points[b], points[c]) <= epsilon)
        throw std::runtime_error("face contains a degenerate triangle");
    face.triangles.push_back({{a, b, c}});
}

// Validation of the values and return a vector of the values
std::vector<float> readValues(std::istringstream& line, const std::string& record)
{
    std::vector<float> result;
    std::string token;
    while (line >> token)
    {
        std::istringstream number(token);
        number.imbue(std::locale::classic());
        float value;
        if (!(number >> value) || number.peek() != EOF || !std::isfinite(value))
            throw std::runtime_error("invalid " + record + " value '" + token + "'");
        result.push_back(value);
    }
    return result;
}

long long readSmoothingGroup(std::istringstream& line)
{
    std::string group, extra;
    if (!(line >> group) || line >> extra)
        throw std::runtime_error("invalid smoothing group");
    if (group == "off")
        return 0;
    if (group == "on")
        return 1;

    std::istringstream number(group);
    number.imbue(std::locale::classic());
    long long id;
    if (!(number >> id) || number.peek() != EOF || id < 0)
        throw std::runtime_error("invalid smoothing group '" + group + "'");
    return id;
}
}

ObjMesh loadObj(const std::string& path, bool smoothByDefault)
{
    std::ifstream file(path);
    if (!file)
        throw std::runtime_error(path + ": cannot open OBJ");

    std::vector<glm::vec3> positions, normals;
    std::vector<glm::vec2> uvs;
    std::vector<Face> faces;
    long long smoothingGroup = smoothByDefault ? -1 : 0;
    std::size_t lineNumber = 0;
    std::string physicalLine, record;
    bool continued = false;

    // Adapted from CSCI580 Raytrace Mesh::Read_Obj.
    while (std::getline(file, physicalLine))
    {
        // Skip the unnecessary lines
        ++lineNumber;
        if (lineNumber == 1 && physicalLine.compare(0, 3, "\xEF\xBB\xBF") == 0)
            physicalLine.erase(0, 3);
        const auto comment = physicalLine.find('#');
        if (comment != std::string::npos)
            physicalLine.erase(comment);
        const auto end = physicalLine.find_last_not_of(" \t\r");
        continued = end != std::string::npos && physicalLine[end] == '\\';
        if (continued)
        {
            record += physicalLine.substr(0, end) + ' ';
            continue;
        }

        // Get the line 
        record += physicalLine;
        std::istringstream line(record);
        line.imbue(std::locale::classic());
        record.clear();
        std::string kind;
        line >> kind;

        // Do value read base on the first word in the line
        try
        {
            // Vertex
            if (kind == "v")
            {
                const auto v = readValues(line, kind);
                if (v.size() != 3 && v.size() != 4 && v.size() != 6)
                    throw std::runtime_error("expected v x y z, optional w or RGB");
                // Optional curve weight and vertex colors do not affect mesh positions.
                positions.emplace_back(v[0], v[1], v[2]);
            }
            // Vertex coordinate coordinate
            else if (kind == "vt")
            {
                const auto v = readValues(line, kind);
                if (v.empty() || v.size() > 3)
                    throw std::runtime_error("expected one to three texture coordinates");
                uvs.emplace_back(v[0], v.size() >= 2 ? v[1] : 0.0f);
            }
            // Vertex normal
            else if (kind == "vn")
            {
                const auto v = readValues(line, kind);
                if (v.size() != 3)
                    throw std::runtime_error("expected vn x y z");
                const glm::dvec3 n(v[0], v[1], v[2]);
                if (glm::dot(n, n) == 0.0)
                    throw std::runtime_error("vertex normal has zero length");
                normals.push_back(glm::vec3(glm::normalize(n)));
            }
            // Smooth grouping
            else if (kind == "s")
            {
                smoothingGroup = readSmoothingGroup(line);
            }
            // Face
            else if (kind == "f")
            {
                Face face;
                face.smoothingGroup = smoothingGroup;
                std::string token;
                while (line >> token)
                    face.corners.push_back(readCorner(token, positions.size(), uvs.size(), normals.size()));
                triangulate(face, positions);
                faces.push_back(std::move(face));
            }
        }
        catch (const std::runtime_error& error)
        {
            throw std::runtime_error(path + ":" + std::to_string(lineNumber) + ": " + error.what());
        }
    }
    if (file.bad() || continued || faces.empty())
    {
        const char* reason = file.bad() ? "read failed" :
            (continued ? "incomplete line continuation" : "no polygon faces found");
        throw std::runtime_error(path + ":" + std::to_string(lineNumber) + ": " + reason);
    }

    // Average face normals by position and smoothing group.
    using SmoothKey = std::pair<long long, std::size_t>;
    std::map<SmoothKey, glm::dvec3> smoothNormals;
    for (const Face& face : faces)
    {
        if (face.smoothingGroup == 0)
            continue;
        for (const Corner& corner : face.corners)
        {
            const auto entry = smoothNormals.emplace(
                SmoothKey(face.smoothingGroup, corner.position), glm::dvec3(0.0));
            entry.first->second += face.areaNormal;
        }
    }

    std::size_t vertexCount = 0;
    for (const Face& face : faces)
        vertexCount += 3 * face.triangles.size();
    ObjMesh mesh;
    mesh.vertices.reserve(vertexCount);
    for (const Face& face : faces)
    {
        const glm::vec3 faceNormal(glm::normalize(face.areaNormal));
        for (const auto& triangle : face.triangles)
        {
            for (std::size_t index : triangle)
            {
                const Corner& corner = face.corners[index];
                glm::vec3 normal = faceNormal;
                if (corner.normal != missing)
                    normal = normals[corner.normal];
                else if (face.smoothingGroup != 0)
                {
                    const auto& sum = smoothNormals.at({face.smoothingGroup, corner.position});
                    if (glm::dot(sum, sum) > 0.0)
                        normal = glm::vec3(glm::normalize(sum));
                }
                const bool hasUV = corner.uv != missing;
                mesh.vertices.push_back({positions[corner.position], normal,
                                         hasUV ? uvs[corner.uv] : glm::vec2(0.0f), hasUV});
            }
        }
    }
    return mesh;
}
