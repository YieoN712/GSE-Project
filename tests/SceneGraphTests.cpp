#include "SceneGraph.h"
#include "LevelOne.h"

#include <cmath>
#include <iostream>
#include <stdexcept>

namespace
{
    void Check(bool condition, const char* message)
    {
        if (!condition)
        {
            throw std::runtime_error(message);
        }
    }

    bool Near(float a, float b)
    {
        return std::abs(a - b) < .0001f;
    }

    class CounterActor : public Actor
    {
    public:
        void Update(float dt) override
        {
            elapsed += dt;
        }

        float elapsed = 0;
    };

    class RemovingActor : public Actor
    {
    public:
        explicit RemovingActor(SceneGraph& owner) : graph(owner)
        {
        }

        void Update(float) override
        {
            graph.Destroy(Id());
        }

        SceneGraph& graph;
    };

    void Hierarchy()
    {
        SceneGraph graph;
        auto& root = graph.Spawn<Actor>(0);
        root.position = {10, 20};
        root.height = 3;
        root.heading = 1.57079632679f;
        root.scale = {2, 3, 4};
        auto& child = graph.Spawn<CounterActor>(root.Id());
        child.position = {1, 2};
        child.height = 1;
        auto world = graph.WorldMatrix(child.Id());
        Check(Near(world[12], 18) && Near(world[13], 6) && Near(world[14], 18),
              "Parent translation, yaw and scale must compose with the local pose.");
        Check(!graph.Reparent(root.Id(), child.Id()), "Cycles must be rejected.");
        Check(!graph.Reparent(child.Id(), child.Id()), "Self-parenting must be rejected.");
        Check(!graph.Reparent(child.Id(), 9999), "Missing parents must be rejected.");
        root.visible = false;
        Check(!graph.IsVisible(child.Id()), "Visibility must be inherited.");
        graph.Update(.2f);
        Check(Near(child.elapsed, .2f), "Hidden actors must still simulate.");
        root.active = false;
        graph.Update(.2f);
        Check(Near(child.elapsed, .2f), "Inactive descendants must not simulate.");
        root.active = root.visible = true;
        root.castsShadow = false;
        int shadows = 0;
        graph.Visit(
            [&](const Actor&, const ActorMatrix&)
            {
                ++shadows;
            },
            true);
        Check(shadows == 0, "Shadow exclusion must propagate to children.");
        auto& laterParent = graph.Spawn<Actor>(0);
        Check(graph.Reparent(root.Id(), laterParent.Id()), "Valid parenting must succeed.");
        std::vector<ActorId> order;
        graph.Visit(
            [&](const Actor& actor, const ActorMatrix&)
            {
                order.push_back(actor.Id());
            });
        Check(order.size() == 3 && order[0] == laterParent.Id() && order[1] == root.Id(),
              "Render traversal must visit parent before child regardless of creation order.");
        ActorId rootId = root.Id(), childId = child.Id(), laterId = laterParent.Id();
        graph.Destroy(rootId);
        Check(!graph.Find(rootId) && !graph.Find(childId) && graph.Find(laterId),
              "Destroy must invalidate precisely the requested subtree immediately.");
        graph.FlushDestroyed();
        Check(graph.Nodes().size() == 1, "Cleanup must release marked nodes.");
        graph.Clear();
        Check(graph.Spawn<Actor>(0).Id() > laterId && !graph.Find(childId),
              "Clear must not recycle stale handles.");
    }

    void Lifetime()
    {
        SceneGraph graph;
        ActorList<DustActor> particles(graph);
        auto& dust = particles.push_back({{2, 3}, 0});
        ActorId id = dust.Id();
        Check(graph.Find(id) == &particles[0], "Typed queries must expose graph-owned state.");
        graph.Update(.5f);
        Check(Near(particles[0].age, .5f), "Each actor must update exactly once.");
        particles.RemoveIf(
            [](const DustActor& actor)
            {
                return actor.age > .4f;
            });
        Check(particles.empty() && !graph.Find(id), "Queries must exclude destroyed nodes.");
        auto& removing = graph.Spawn<RemovingActor>(0, graph);
        ActorId removingId = removing.Id();
        graph.Spawn<CounterActor>(removingId);
        graph.Update(.1f);
        Check(!graph.Find(removingId) && graph.Nodes().empty(),
              "Self-destruction during update must safely remove descendants.");
        auto& actor = graph.Spawn<Actor>(0);
        bool blocked = false;
        graph.Visit(
            [&](const Actor&, const ActorMatrix&)
            {
                try
                {
                    graph.FlushDestroyed();
                }
                catch (const std::logic_error&)
                {
                    blocked = true;
                }
            });
        Check(blocked && graph.Find(actor.Id()), "Cleanup during traversal must be rejected.");
    }

    void LevelOwnership()
    {
        LevelOne level(7);
        Check(level.scene.Find(level.Player().Id()) == &level.Player(),
              "The player's authoritative state must be owned by SceneGraph.");
        Check(level.scene.Find(level.enemies[0].Id()) == &level.enemies[0],
              "Combat and rendering must reference the same enemy.");
        auto count = level.scene.Nodes().size();
        ActorId oldEnemy = level.enemies[0].Id();
        level.Reset(7);
        Check(level.scene.Nodes().size() == count && !level.scene.Find(oldEnemy),
              "Reset must replace old actors and attached effects without duplication.");
        ActorId playerId = level.Player().Id();
        level.scene.Destroy(playerId);
        level.scene.FlushDestroyed();
        level.Update(.01f);
        Check(!level.FindPlayer(), "Deleting the player must not leave a dangling reference.");
        level.KeyDown('r');
        level.KeyUp('r');
        Check(level.FindPlayer() && level.Player().Id() != playerId,
              "Restart must recreate a removed player with a fresh handle.");
        Check(level.AllGroundConnected(), "Actor migration must preserve connected terrain.");
    }
} // namespace

int main()
{
    try
    {
        Hierarchy();
        Lifetime();
        LevelOwnership();
        std::cout << "SceneGraph checks passed.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << "\n";
        return 1;
    }
}
