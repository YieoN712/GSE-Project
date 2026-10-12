#include "Actor.h"

#include <cmath>

ActorMatrix IdentityMatrix()
{
    return {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
}

ActorMatrix Multiply(const ActorMatrix& a, const ActorMatrix& b)
{
    ActorMatrix result{};
    for (int column = 0; column < 4; ++column)
    {
        for (int row = 0; row < 4; ++row)
        {
            for (int k = 0; k < 4; ++k)
            {
                result[column * 4 + row] += a[k * 4 + row] * b[column * 4 + k];
            }
        }
    }
    return result;
}

void Actor::Update(float)
{
}

ActorMatrix Actor::LocalMatrix() const
{
    float c = std::cos(heading);
    float s = std::sin(heading);
    return {c * scale.x,
            0,
            -s * scale.x,
            0,
            0,
            scale.y,
            0,
            0,
            s * scale.z,
            0,
            c * scale.z,
            0,
            position.x,
            height,
            position.z,
            1};
}

ActorId Actor::Id() const
{
    return id;
}

ActorId Actor::Parent() const
{
    return parent;
}

DustActor::DustActor(WorldPoint point, float elapsed) : age(elapsed)
{
    position = point;
    name = "Footstep";
    castsShadow = false;
}

void DustActor::Update(float dt)
{
    age += dt;
}
