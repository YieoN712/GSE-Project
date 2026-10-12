#include "SceneGraph.h"

namespace
{
    struct TraversalScope
    {
        explicit TraversalScope(int& value) : depth(value)
        {
            ++depth;
        }

        ~TraversalScope()
        {
            --depth;
        }

        int& depth;
    };
} // namespace

Actor* SceneGraph::Find(ActorId id)
{
    auto found = nodes.find(id);
    return found == nodes.end() || found->second->destroyed ? nullptr : found->second.get();
}

const Actor* SceneGraph::Find(ActorId id) const
{
    auto found = nodes.find(id);
    return found == nodes.end() || found->second->destroyed ? nullptr : found->second.get();
}

bool SceneGraph::Reparent(ActorId child, ActorId parent)
{
    Actor* actor = Find(child);
    if (!actor || (parent && !Find(parent)))
    {
        return false;
    }
    for (ActorId ancestor = parent; ancestor; ancestor = Find(ancestor)->Parent())
    {
        if (ancestor == child)
        {
            return false;
        }
    }
    // Keep-local semantics: changing parent deliberately changes the world pose.
    actor->parent = parent;
    return true;
}

void SceneGraph::Destroy(ActorId id)
{
    Actor* actor = Find(id);
    if (!actor)
    {
        return;
    }
    actor->destroyed = true;
    for (const auto& pair : nodes)
    {
        if (pair.second->Parent() == id)
        {
            Destroy(pair.first);
        }
    }
}

void SceneGraph::FlushDestroyed()
{
    if (traversalDepth)
    {
        throw std::logic_error("FlushDestroyed must run outside graph traversal.");
    }
    for (auto it = nodes.begin(); it != nodes.end();)
    {
        if (it->second->destroyed)
        {
            it = nodes.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void SceneGraph::Clear()
{
    if (traversalDepth)
    {
        throw std::logic_error("Clear must run outside graph traversal.");
    }
    nodes.clear();
    // Do not reuse IDs: handles retained by views cannot point at a new actor after reset.
}

bool SceneGraph::IsActive(ActorId id) const
{
    while (id)
    {
        const Actor* actor = Find(id);
        if (!actor || !actor->active)
        {
            return false;
        }
        id = actor->Parent();
    }
    return true;
}

bool SceneGraph::IsVisible(ActorId id) const
{
    while (id)
    {
        const Actor* actor = Find(id);
        if (!actor || !actor->active || !actor->visible)
        {
            return false;
        }
        id = actor->Parent();
    }
    return true;
}

ActorMatrix SceneGraph::WorldMatrix(ActorId id) const
{
    const Actor* actor = Find(id);
    if (!actor)
    {
        return IdentityMatrix();
    }
    return Multiply(WorldMatrix(actor->Parent()), actor->LocalMatrix());
}

void SceneGraph::Update(float dt)
{
    if (traversalDepth)
    {
        throw std::logic_error("Scene update must not be nested in graph traversal.");
    }
    {
        TraversalScope scope(traversalDepth);
        std::map<ActorId, std::vector<ActorId>> snapshot;
        for (const auto& pair : nodes)
        {
            if (Find(pair.first))
            {
                snapshot[pair.second->Parent()].push_back(pair.first);
            }
        }
        std::function<void(ActorId)> walk;
        walk = [&](ActorId parent)
        {
            auto children = snapshot.find(parent);
            if (children == snapshot.end())
            {
                return;
            }
            for (ActorId id : children->second)
            {
                if (!IsActive(id))
                {
                    continue;
                }
                Find(id)->Update(dt);
                walk(id);
            }
        };
        walk(0);
    }
    FlushDestroyed();
}

void SceneGraph::Visit(const std::function<void(const Actor&, const ActorMatrix&)>& visitor,
                       bool shadowPass) const
{
    TraversalScope scope(traversalDepth);
    std::map<ActorId, std::vector<const Actor*>> children;
    for (const auto& pair : nodes)
    {
        if (Find(pair.first))
        {
            children[pair.second->Parent()].push_back(pair.second.get());
        }
    }
    std::function<void(ActorId, const ActorMatrix&, bool)> walk;
    walk = [&](ActorId parent, const ActorMatrix& matrix, bool parentShadow)
    {
        auto found = children.find(parent);
        if (found == children.end())
        {
            return;
        }
        for (const Actor* actor : found->second)
        {
            if (!IsVisible(actor->Id()))
            {
                continue;
            }
            ActorMatrix world = Multiply(matrix, actor->LocalMatrix());
            bool shadow = parentShadow && actor->castsShadow;
            if (!shadowPass || shadow)
            {
                visitor(*actor, world);
            }
            walk(actor->Id(), world, shadow);
        }
    };
    walk(0, IdentityMatrix(), true);
}

const SceneGraph::Storage& SceneGraph::Nodes() const
{
    return nodes;
}
