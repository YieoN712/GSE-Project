#pragma once

#include <array>
#include <cstdint>
#include <string>

using ActorId = std::uint64_t;

struct WorldPoint
{
    float x = 0;
    float z = 0;
};

struct ActorScale
{
    float x = 1;
    float y = 1;
    float z = 1;
};

using ActorMatrix = std::array<float, 16>;
ActorMatrix IdentityMatrix();
ActorMatrix Multiply(const ActorMatrix& a, const ActorMatrix& b);

// CPU-only world entity. Position/height/heading/scale are local to its parent.
class Actor
{
public:
    virtual ~Actor() = default;
    virtual void Update(float dt);
    ActorMatrix LocalMatrix() const;
    ActorId Id() const;
    ActorId Parent() const;

    std::string name;
    WorldPoint position;
    float height = 0;
    float heading = 0;
    ActorScale scale;
    bool active = true;
    bool visible = true;
    bool castsShadow = true;

private:
    friend class SceneGraph;
    ActorId id = 0;
    ActorId parent = 0;
    bool destroyed = false;
};

class PlayerActor : public Actor
{
public:
    float stride = 0;
    float speed = 0;
};

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

class MeshActor : public Actor
{
public:
    ModelKind model = ModelKind::Box;
    float r = 1, g = 1, b = 1;
    int material = -1;
};

class DustActor : public Actor
{
public:
    DustActor(WorldPoint point = {}, float elapsed = 0);
    void Update(float dt) override;
    float age = 0;
};

enum class MarkerKind
{
    Player,
    Attack,
    Spawn,
    Enemy,
    Pickup,
    Objective,
    Target,
    Anomaly
};

class MarkerActor : public Actor
{
public:
    explicit MarkerActor(MarkerKind value = MarkerKind::Player) : kind(value)
    {
        name = "World marker";
        castsShadow = false;
    }

    MarkerKind kind;
};
