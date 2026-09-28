#pragma once

#include "Dependencies/glew.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

class RenderEffects;

enum class ModelKind : std::uint32_t
{
    Box,
    Sphere,
    Shop,
    Home,
    Apartments,
    Utility,
    Tree,
    Water,
    Leaf,
    Potion,
    Shard,
    Weapon,
    Count
};

class ModelCache
{
public:
    struct Vertex
    {
        float position[3];
        float normal[3];
        float color[4];
    };

    struct Part
    {
        std::uint32_t material = 0;
        std::vector<Vertex> vertices;
        GLuint buffer = 0;
    };

    ModelCache();
    ~ModelCache();
    ModelCache(const ModelCache&) = delete;
    ModelCache& operator=(const ModelCache&) = delete;

    void Draw(ModelKind kind,
              RenderEffects& effects,
              float x,
              float y,
              float z,
              float sx = 1,
              float sy = 1,
              float sz = 1,
              float heading = 0,
              float r = 1,
              float g = 1,
              float b = 1,
              int overrideMaterial = -1);

    bool loadedFromDisk = false;
    bool savedToDisk = false;
    std::wstring cachePath;

private:
    std::array<std::vector<Part>, static_cast<size_t>(ModelKind::Count)> meshes;
    bool Load();
    bool Save() const;
    void Generate();
    void Upload();
};
