#pragma once

#include "Actor.h"

#include <functional>
#include <initializer_list>
#include <map>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

class SceneGraph
{
public:
    using Storage = std::map<ActorId, std::unique_ptr<Actor>>;
    SceneGraph() = default;
    SceneGraph(const SceneGraph&) = delete;
    SceneGraph& operator=(const SceneGraph&) = delete;

    template <class T, class... Args> T& Spawn(ActorId parent, Args&&... args)
    {
        static_assert(std::is_base_of<Actor, T>::value, "Scene nodes must be Actors.");
        if (parent && !Find(parent))
        {
            throw std::invalid_argument("Missing actor parent.");
        }
        auto actor = std::make_unique<T>(std::forward<Args>(args)...);
        actor->id = nextId++;
        actor->parent = parent;
        actor->destroyed = false;
        T& result = *actor;
        nodes.emplace(actor->id, std::move(actor));
        return result;
    }

    Actor* Find(ActorId id);
    const Actor* Find(ActorId id) const;
    bool Reparent(ActorId child, ActorId parent);
    void Destroy(ActorId id);
    void FlushDestroyed();
    void Clear();
    void Update(float dt);
    bool IsActive(ActorId id) const;
    bool IsVisible(ActorId id) const;
    ActorMatrix WorldMatrix(ActorId id) const;
    void Visit(const std::function<void(const Actor&, const ActorMatrix&)>& visitor,
               bool shadowPass = false) const;
    const Storage& Nodes() const;

private:
    Storage nodes;
    ActorId nextId = 1;
    mutable int traversalDepth = 0;
};

// Non-owning typed query. All allocation and destruction goes through SceneGraph.
// Iteration is stable across Spawn/Destroy; FlushDestroyed is a frame-boundary operation.
template <class T> class ActorList
{
public:
    explicit ActorList(SceneGraph& owner) : graph(owner)
    {
    }

    template <bool Constant> class Iterator
    {
    public:
        using Reference = typename std::conditional<Constant, const T&, T&>::type;

        Iterator(SceneGraph& owner, SceneGraph::Storage::const_iterator value)
            : graph(&owner), current(value)
        {
            Skip();
        }

        Reference operator*() const
        {
            return *static_cast<T*>(current->second.get());
        }

        Iterator& operator++()
        {
            ++current;
            Skip();
            return *this;
        }

        bool operator!=(const Iterator& other) const
        {
            return current != other.current;
        }

        bool operator==(const Iterator& other) const
        {
            return current == other.current;
        }

    private:
        void Skip()
        {
            while (current != graph->Nodes().end() &&
                   (!graph->Find(current->first) || !dynamic_cast<T*>(current->second.get())))
            {
                ++current;
            }
        }

        SceneGraph* graph;
        SceneGraph::Storage::const_iterator current;
    };

    Iterator<false> begin()
    {
        return {graph, graph.Nodes().begin()};
    }

    Iterator<false> end()
    {
        return {graph, graph.Nodes().end()};
    }

    Iterator<true> begin() const
    {
        return {graph, graph.Nodes().begin()};
    }

    Iterator<true> end() const
    {
        return {graph, graph.Nodes().end()};
    }

    bool empty() const
    {
        return begin() == end();
    }

    size_t size() const
    {
        size_t count = 0;
        for (const auto& actor : *this)
        {
            (void)actor;
            ++count;
        }
        return count;
    }

    T& operator[](size_t index)
    {
        for (auto& actor : *this)
        {
            if (index-- == 0)
            {
                return actor;
            }
        }
        throw std::out_of_range("Actor index");
    }

    const T& operator[](size_t index) const
    {
        for (const auto& actor : *this)
        {
            if (index-- == 0)
            {
                return actor;
            }
        }
        throw std::out_of_range("Actor index");
    }

    T& push_back(const T& actor)
    {
        return graph.Spawn<T>(0, actor);
    }

    void erase_at(size_t index)
    {
        graph.Destroy((*this)[index].Id());
    }

    template <class Predicate> void RemoveIf(Predicate predicate)
    {
        for (auto& actor : *this)
        {
            if (predicate(actor))
            {
                graph.Destroy(actor.Id());
            }
        }
    }

    void clear()
    {
        RemoveIf(
            [](const T&)
            {
                return true;
            });
    }

    ActorList& operator=(std::initializer_list<T> actors)
    {
        clear();
        for (const auto& actor : actors)
        {
            push_back(actor);
        }
        return *this;
    }

private:
    SceneGraph& graph;
};
