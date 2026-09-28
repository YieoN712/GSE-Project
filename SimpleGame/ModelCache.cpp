#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>

#include "ModelCache.h"
#include "RenderEffects.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <type_traits>

namespace
{
    constexpr std::uint32_t Magic = 0x4753454d;
    constexpr std::uint32_t Version = 1;
    using Vertex = ModelCache::Vertex;
    using Part = ModelCache::Part;
    static_assert(sizeof(Vertex) == 40, "Cache vertex layout must remain stable.");
    static_assert(std::is_trivially_copyable<Vertex>::value, "Cache vertices must be plain data.");

    std::uint32_t Checksum(const std::vector<Vertex>& vertices)
    {
        std::uint32_t hash = 2166136261u;
        const auto* bytes = reinterpret_cast<const unsigned char*>(vertices.data());

        for (size_t i = 0; i < vertices.size() * sizeof(Vertex); ++i)
        {
            hash = (hash ^ bytes[i]) * 16777619u;
        }

        return hash;
    }

    template <class T> bool Read(std::istream& in, T& value)
    {
        return bool(in.read(reinterpret_cast<char*>(&value), sizeof(T)));
    }

    template <class T> void Write(std::ostream& out, const T& value)
    {
        out.write(reinterpret_cast<const char*>(&value), sizeof(T));
    }

    Part& Material(std::vector<Part>& mesh, unsigned material)
    {
        for (auto& part : mesh)
        {
            if (part.material == material)
            {
                return part;
            }
        }

        mesh.push_back({material, {}, 0});
        return mesh.back();
    }

    void Quad(Part& part,
              const std::array<std::array<float, 3>, 4>& points,
              float nx,
              float ny,
              float nz,
              float r,
              float g,
              float b)
    {
        for (int index : {0, 1, 2, 0, 2, 3})
        {
            const auto& point = points[index];
            part.vertices.push_back({{point[0], point[1], point[2]}, {nx, ny, nz}, {r, g, b, 1}});
        }
    }

    void Box(std::vector<Part>& mesh,
             float x,
             float y,
             float z,
             float w,
             float h,
             float d,
             float r,
             float g,
             float b,
             unsigned material)
    {
        Part& part = Material(mesh, material);
        float a = x - w / 2, c = x + w / 2;
        float e = z - d / 2, f = z + d / 2;
        float t = y + h;
        Quad(part, {{{a, y, f}, {c, y, f}, {c, t, f}, {a, t, f}}}, 0, 0, 1, r, g, b);
        Quad(part, {{{c, y, e}, {a, y, e}, {a, t, e}, {c, t, e}}}, 0, 0, -1, r, g, b);
        Quad(part, {{{a, y, e}, {a, y, f}, {a, t, f}, {a, t, e}}}, -1, 0, 0, r, g, b);
        Quad(part, {{{c, y, f}, {c, y, e}, {c, t, e}, {c, t, f}}}, 1, 0, 0, r, g, b);
        Quad(part, {{{a, t, e}, {a, t, f}, {c, t, f}, {c, t, e}}}, 0, 1, 0, r, g, b);
        Quad(part, {{{a, y, f}, {a, y, e}, {c, y, e}, {c, y, f}}}, 0, -1, 0, r, g, b);
    }

    void Sphere(std::vector<Part>& mesh,
                float x,
                float y,
                float z,
                float rx,
                float ry,
                float rz,
                float r,
                float g,
                float b,
                unsigned material)
    {
        Part& part = Material(mesh, material);

        for (int row = 0; row < 10; ++row)
        {
            for (int column = 0; column < 16; ++column)
            {
                Vertex corners[4]{};

                for (int i = 0; i < 4; ++i)
                {
                    float latitude = (row + (i >= 2 ? 1 : 0)) * 3.14159265f / 10 - 1.57079633f;
                    float longitude = (column + (i == 1 || i == 2 ? 1 : 0)) * 6.2831853f / 16;
                    float nx = std::cos(latitude) * std::cos(longitude);
                    float ny = std::sin(latitude);
                    float nz = std::cos(latitude) * std::sin(longitude);
                    float length =
                        std::sqrt(nx * nx / (rx * rx) + ny * ny / (ry * ry) + nz * nz / (rz * rz));
                    corners[i] = {{x + rx * nx, y + ry * ny, z + rz * nz},
                                  {nx / rx / length, ny / ry / length, nz / rz / length},
                                  {r, g, b, 1}};
                }

                for (int i : {0, 2, 1, 0, 3, 2})
                {
                    part.vertices.push_back(corners[i]);
                }
            }
        }
    }

    std::wstring CacheLocation()
    {
        wchar_t local[32768]{};
        DWORD size = GetEnvironmentVariableW(L"LOCALAPPDATA", local, 32768);
        std::filesystem::path directory;
        std::error_code error;

        if (size > 0 && size < 32768)
        {
            directory = std::filesystem::path(local) / L"GSEProject" / L"cache";
            std::filesystem::create_directories(directory, error);
        }

        if (directory.empty() || error)
        {
            wchar_t executable[32768]{};
            DWORD length = GetModuleFileNameW(nullptr, executable, 32768);

            if (length == 0 || length >= 32768)
            {
                return L"";
            }

            directory = std::filesystem::path(executable).parent_path() / L"cache";
            std::filesystem::create_directories(directory, error);

            if (error)
            {
                return L"";
            }
        }

        return (directory / L"level1_models_v1.bin").wstring();
    }
} // namespace

ModelCache::ModelCache()
{
    cachePath = CacheLocation();
    loadedFromDisk = Load();

    if (!loadedFromDisk)
    {
        for (auto& mesh : meshes)
        {
            mesh.clear();
        }

        Generate();
        savedToDisk = Save();
    }

    Upload();
    std::wcout << L"Model cache path: " << cachePath << L"\n";
    std::cout << (loadedFromDisk ? "Model cache: loaded; generation skipped.\n"
                  : savedToDisk  ? "Model cache: generated and saved.\n"
                                 : "Model cache: memory-only (disk write unavailable).\n");
}

ModelCache::~ModelCache()
{
    for (auto& mesh : meshes)
    {
        for (auto& part : mesh)
        {
            if (part.buffer)
            {
                glDeleteBuffers(1, &part.buffer);
            }
        }
    }
}

bool ModelCache::Load()
{
    if (cachePath.empty())
    {
        return false;
    }

    std::ifstream in(std::filesystem::path(cachePath), std::ios::binary);
    std::uint32_t magic = 0, version = 0, models = 0;

    if (!Read(in, magic) || !Read(in, version) || !Read(in, models) || magic != Magic ||
        version != Version || models != meshes.size())
    {
        return false;
    }

    size_t totalVertices = 0;

    for (auto& mesh : meshes)
    {
        std::uint32_t parts = 0;

        if (!Read(in, parts) || parts == 0 || parts > 16)
        {
            return false;
        }

        mesh.resize(parts);

        for (auto& part : mesh)
        {
            std::uint32_t count = 0, checksum = 0;

            if (!Read(in, part.material) || !Read(in, count) || !Read(in, checksum) ||
                part.material > 11 || count == 0 || count % 3 != 0 || count > 100000)
            {
                return false;
            }

            totalVertices += count;

            if (totalVertices > 500000)
            {
                return false;
            }

            part.vertices.resize(count);
            in.read(reinterpret_cast<char*>(part.vertices.data()), count * sizeof(Vertex));

            if (!in || Checksum(part.vertices) != checksum)
            {
                return false;
            }

            for (const auto& vertex : part.vertices)
            {
                for (float value : vertex.position)
                {
                    if (!std::isfinite(value) || std::abs(value) > 100)
                    {
                        return false;
                    }
                }

                for (float value : vertex.normal)
                {
                    if (!std::isfinite(value) || std::abs(value) > 1.01f)
                    {
                        return false;
                    }
                }

                for (float value : vertex.color)
                {
                    if (!std::isfinite(value) || value < 0 || value > 1)
                    {
                        return false;
                    }
                }
            }
        }
    }

    return in.peek() == std::char_traits<char>::eof();
}

bool ModelCache::Save() const
{
    if (cachePath.empty())
    {
        return false;
    }

    std::wstring temporary = cachePath + L"." + std::to_wstring(GetCurrentProcessId()) + L".tmp";
    std::ofstream out(std::filesystem::path(temporary), std::ios::binary | std::ios::trunc);
    Write(out, Magic);
    Write(out, Version);
    Write(out, static_cast<std::uint32_t>(meshes.size()));

    for (const auto& mesh : meshes)
    {
        Write(out, static_cast<std::uint32_t>(mesh.size()));

        for (const auto& part : mesh)
        {
            Write(out, part.material);
            Write(out, static_cast<std::uint32_t>(part.vertices.size()));
            Write(out, Checksum(part.vertices));
            out.write(reinterpret_cast<const char*>(part.vertices.data()),
                      part.vertices.size() * sizeof(Vertex));
        }
    }

    out.close();

    if (!out || !MoveFileExW(temporary.c_str(),
                             cachePath.c_str(),
                             MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
    {
        DeleteFileW(temporary.c_str());
        return false;
    }

    return true;
}

void ModelCache::Generate()
{
    auto mesh = [&](ModelKind kind) -> std::vector<Part>&
    {
        return meshes[static_cast<size_t>(kind)];
    };
    Box(mesh(ModelKind::Box), 0, 0, 0, 1, 1, 1, 1, 1, 1, 0);
    Sphere(mesh(ModelKind::Sphere), 0, 0, 0, 1, 1, 1, 1, 1, 1, 8);

    for (ModelKind kind :
         {ModelKind::Shop, ModelKind::Home, ModelKind::Apartments, ModelKind::Utility})
    {
        auto& model = mesh(kind);
        bool shop = kind == ModelKind::Shop;
        bool apartment = kind == ModelKind::Apartments;
        float height = apartment ? 5.8f : (kind == ModelKind::Utility ? 2.6f : 3.4f);
        Box(model, 0, 0, 0, 1, height, 1, shop ? .93f : .80f, .80f, .70f, 4);
        Box(model, 0, height, 0, 1.04f, .16f, 1.04f, .40f, .46f, .47f, 5);
        Box(model, 0, 0, .503f, .16f, 1.9f, .02f, .30f, .40f, .42f, 6);

        for (int row = 0; row < (apartment ? 3 : 1); ++row)
        {
            for (float x : {-.33f, .33f})
            {
                Box(model, x, 1.2f + row * 1.5f, .509f, .20f, .9f, .02f, .35f, .56f, .62f, 6);
                Box(model, x, 1.18f + row * 1.5f, .53f, .22f, .07f, .07f, .84f, .84f, .77f, 5);
            }
        }

        if (shop)
        {
            Box(model, 0, 2.35f, .59f, .93f, .12f, .25f, .25f, .57f, .54f, 8);

            for (int i = 0; i < 6; ++i)
            {
                Box(model, -.38f + i * .15f, 2.35f, .59f, .06f, .125f, .25f, .94f, .88f, .71f, 8);
            }
        }
        else if (kind == ModelKind::Home)
        {
            // Stepped roof, chimney and a narrow garden planter; no awning.
            Box(model, 0, height + .16f, 0, .72f, .35f, 1, .60f, .39f, .31f, 5);
            Box(model, .30f, height + .15f, -.20f, .12f, .9f, .15f, .70f, .60f, .47f, 4);
        }
        else if (apartment)
        {
            Box(model, 0, 3.1f, .57f, .85f, .10f, .16f, .54f, .59f, .57f, 3);
            Box(model, 0, 3.2f, .65f, .85f, .42f, .025f, .45f, .53f, .52f, 5);
        }
        else
        {
            Box(model, 0, height + .16f, -.10f, .44f, .55f, .4f, .70f, .75f, .73f, 5);
            for (int i = 0; i < 5; ++i)
            {
                Box(model, .29f, .7f + i * .15f, .518f, .28f, .055f, .025f, .37f, .44f, .45f, 5);
            }
        }
    }

    auto& tree = mesh(ModelKind::Tree);
    Box(tree, 0, 0, 0, .28f, 2.6f, .28f, .49f, .37f, .24f, 7);
    Sphere(tree, 0, 2.9f, 0, 1.2f, 1.1f, 1.15f, .39f, .62f, .34f, 9);
    Sphere(tree, .25f, 3.7f, -.10f, .82f, .72f, .80f, .56f, .72f, .41f, 9);

    auto& water = Material(mesh(ModelKind::Water), 10);
    for (int z = 0; z < 16; ++z)
    {
        for (int x = 0; x < 16; ++x)
        {
            float a = x / 16.f - .5f, b = (x + 1) / 16.f - .5f;
            float c = z / 16.f - .5f, d = (z + 1) / 16.f - .5f;
            Quad(water, {{{a, 0, c}, {a, 0, d}, {b, 0, d}, {b, 0, c}}}, 0, 1, 0, .28f, .56f, .61f);
        }
    }

    auto& leaf = Material(mesh(ModelKind::Leaf), 11);
    Quad(leaf,
         {{{-.14f, 0, 0}, {0, .03f, .08f}, {.14f, 0, 0}, {0, -.03f, -.08f}}},
         0,
         1,
         0,
         .69f,
         .73f,
         .32f);
    Box(mesh(ModelKind::Potion), 0, 0, 0, .26f, .40f, .26f, .29f, .74f, .62f, 6);
    Box(mesh(ModelKind::Potion), 0, .40f, 0, .16f, .10f, .16f, .93f, .87f, .67f, 5);
    Sphere(mesh(ModelKind::Shard), 0, .2f, 0, .16f, .28f, .16f, .78f, .62f, .94f, 6);
    Box(mesh(ModelKind::Weapon), 0, 0, 0, .13f, .52f, .13f, .38f, .43f, .43f, 5);
    Box(mesh(ModelKind::Weapon), 0, .52f, 0, .22f, .12f, .22f, .97f, .88f, .52f, 6);
}

void ModelCache::Upload()
{
    for (auto& mesh : meshes)
    {
        for (auto& part : mesh)
        {
            glGenBuffers(1, &part.buffer);
            glBindBuffer(GL_ARRAY_BUFFER, part.buffer);
            glBufferData(GL_ARRAY_BUFFER,
                         part.vertices.size() * sizeof(Vertex),
                         part.vertices.data(),
                         GL_STATIC_DRAW);
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void ModelCache::Draw(ModelKind kind,
                      RenderEffects& effects,
                      float x,
                      float y,
                      float z,
                      float sx,
                      float sy,
                      float sz,
                      float heading,
                      float r,
                      float g,
                      float b,
                      int overrideMaterial)
{
    float c = std::cos(heading), s = std::sin(heading);
    const float transform[16] = {
        c * sx, 0, -s * sx, 0, 0, sy, 0, 0, s * sz, 0, c * sz, 0, x, y, z, 1};
    effects.ObjectTransform(transform, r, g, b);
    glPushMatrix();
    glMultMatrixf(transform);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    for (const auto& part : meshes[static_cast<size_t>(kind)])
    {
        effects.Material(overrideMaterial >= 0 ? overrideMaterial : int(part.material));
        glBindBuffer(GL_ARRAY_BUFFER, part.buffer);
        glVertexPointer(
            3, GL_FLOAT, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, position)));
        glNormalPointer(
            GL_FLOAT, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, normal)));
        glColorPointer(
            4, GL_FLOAT, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, color)));
        glDrawArrays(GL_TRIANGLES, 0, GLsizei(part.vertices.size()));
    }

    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glPopMatrix();
    effects.ObjectTransform(nullptr);
}
