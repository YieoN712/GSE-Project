#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>

#include "LevelOneView.h"
#include "LevelOne.h"
#include "ModelCache.h"
#include "RenderEffects.h"

#include <algorithm>
#include <cmath>
#include "LevelOneUi.h"

namespace
{
    constexpr float Pi = 3.14159265f;

    void Circle(LevelPoint point, float y, float radius, float r, float g, float b, float alpha = 1)
    {
        glColor4f(r, g, b, alpha);
        glBegin(GL_LINE_LOOP);

        for (int i = 0; i < 40; ++i)
        {
            float angle = i * Pi / 20;
            glVertex3f(point.x + std::sin(angle) * radius, y, point.z + std::cos(angle) * radius);
        }

        glEnd();
    }

} // namespace

struct LevelOneView::Implementation
{
    RenderEffects effects;
    ModelCache models;
    LevelOneUi ui;

    void Character(LevelPoint point,
                   float heading,
                   float stride,
                   bool walking,
                   float attack,
                   bool enemy,
                   int kind,
                   bool flash,
                   bool armed = true)
    {
        float swing = walking ? std::sin(stride) * .20f : 0;
        float bounce = walking ? std::abs(std::sin(stride)) * .04f : 0;
        float r = enemy ? .45f + kind * .07f : kind == 0 ? .25f : .40f + (kind % 3) * .16f;
        float g = enemy ? .40f : kind == 0 ? .64f : .40f + (kind % 4) * .10f;
        float b = enemy ? .63f : kind == 0 ? .69f : .38f + (kind % 2) * .22f;

        if (flash)
        {
            r = 1;
            g = .80f;
            b = .64f;
        }

        auto body = [&](float x,
                        float y,
                        float z,
                        float sx,
                        float sy,
                        float sz,
                        float red,
                        float green,
                        float blue)
        {
            float wx = point.x + x * std::cos(heading) + z * std::sin(heading);
            float wz = point.z - x * std::sin(heading) + z * std::cos(heading);
            models.Draw(
                ModelKind::Sphere, effects, wx, y, wz, sx, sy, sz, heading, red, green, blue);
        };

        for (int side : {-1, 1})
        {
            body(side * .14f, .11f, swing * side + .06f, .12f, .09f, .19f, .20f, .24f, .28f);
            body(side * .14f, .34f, swing * side * .6f, .11f, .24f, .12f, .26f, .30f, .36f);
            body(side * .32f, .88f + bounce, -swing * side * .5f, .11f, .21f, .12f, r, g, b);
            body(side * .35f, .68f + bounce, -swing * side, .08f, .12f, .09f, .91f, .73f, .57f);
        }

        body(0, .85f + bounce, 0, .28f, .32f, .19f, r, g, b);
        body(0,
             1.37f + bounce,
             0,
             .23f,
             .26f,
             .21f,
             enemy ? .61f : .95f,
             enemy ? .61f : .76f,
             enemy ? .73f : .60f);
        body(0, 1.54f + bounce, -.03f, .24f, .14f, .21f, .24f, .24f, .29f);

        for (int side : {-1, 1})
        {
            body(side * .085f,
                 1.43f + bounce,
                 .197f,
                 .022f,
                 .025f,
                 .024f,
                 enemy ? .94f : .12f,
                 enemy ? .75f : .17f,
                 enemy ? .43f : .20f);
        }

        if (!enemy && armed)
        {
            float swingAngle = heading + (attack > 0 ? (1 - attack / .36f) * 2.3f - 1.15f : .5f);
            models.Draw(ModelKind::Weapon,
                        effects,
                        point.x + std::sin(swingAngle) * .6f,
                        .65f,
                        point.z + std::cos(swingAngle) * .6f,
                        1,
                        1,
                        1,
                        swingAngle);
        }
    }

    void World(const LevelOne& game)
    {
        game.scene.Visit(
            [&](const Actor& actor, const ActorMatrix& world)
            {
                models.SetParentMatrix(world);
                if (const auto* mesh = dynamic_cast<const MeshActor*>(&actor))
                {
                    const Actor* tree =
                        dynamic_cast<const LevelTree*>(&actor)
                            ? &actor
                            : dynamic_cast<const LevelTree*>(game.scene.Find(actor.Parent()));
                    if (tree)
                    {
                        auto treeWorld = game.scene.WorldMatrix(tree->Id());
                        if (std::hypot(treeWorld[12] - game.Player().position.x,
                                       treeWorld[14] - game.Player().position.z) < 2)
                        {
                            return;
                        }
                    }
                    models.Draw(mesh->model,
                                effects,
                                0,
                                0,
                                0,
                                1,
                                1,
                                1,
                                0,
                                mesh->r,
                                mesh->g,
                                mesh->b,
                                mesh->material);
                }
                else if (const auto* building = dynamic_cast<const LevelBuilding*>(&actor))
                {
                    float forward = (world[12] - game.Player().position.x + world[14] -
                                     game.Player().position.z) *
                                    .7071f;
                    float sideways = std::abs((world[12] - game.Player().position.x - world[14] +
                                               game.Player().position.z) *
                                              .7071f);
                    bool cutaway = forward > 0 && forward < 13 &&
                                   sideways < (building->columns + building->rows) * .72f;
                    ModelKind model = ModelKind::Home;
                    switch (building->style)
                    {
                    case BuildingStyle::Shop:
                        model = ModelKind::Shop;
                        break;
                    case BuildingStyle::Home:
                        model = ModelKind::Home;
                        break;
                    case BuildingStyle::Apartments:
                        model = ModelKind::Apartments;
                        break;
                    case BuildingStyle::Utility:
                        model = ModelKind::Utility;
                        break;
                    }
                    models.Draw(cutaway ? ModelKind::Box : model,
                                effects,
                                0,
                                0,
                                0,
                                building->columns * 2.f - .4f,
                                cutaway ? .25f : 1,
                                building->rows * 2.f - .4f,
                                0,
                                cutaway ? .82f : 1,
                                cutaway ? .80f : 1,
                                cutaway ? .70f : 1,
                                cutaway ? 4 : -1);
                }
                else if (const auto* resident = dynamic_cast<const LevelResident*>(&actor))
                {
                    Character({},
                              0,
                              resident->stride,
                              resident->speed > .02f,
                              0,
                              false,
                              resident->kind + 1,
                              false,
                              false);
                }
                else if (const auto* enemy = dynamic_cast<const LevelEnemy*>(&actor))
                {
                    if (enemy->state != EnemyState::Dead)
                    {
                        Character({},
                                  0,
                                  enemy->stride,
                                  enemy->state == EnemyState::Roaming ||
                                      enemy->state == EnemyState::Chasing,
                                  0,
                                  true,
                                  enemy->kind,
                                  enemy->hitFlash > 0);
                    }
                }
                else if (dynamic_cast<const PlayerActor*>(&actor))
                {
                    Character({},
                              0,
                              game.Player().stride,
                              game.Player().speed > .02f,
                              game.attackTime,
                              false,
                              0,
                              game.invulnerable > 0 && int(game.time * 16) % 2 == 0);
                }
                else if (const auto* drop = dynamic_cast<const LevelDrop*>(&actor))
                {
                    ModelKind kind = drop->kind == DropKind::Potion   ? ModelKind::Potion
                                     : drop->kind == DropKind::Memory ? ModelKind::Shard
                                                                      : ModelKind::Weapon;
                    models.Draw(kind,
                                effects,
                                0,
                                .14f + std::sin(game.time * 2) * .07f,
                                0,
                                1,
                                1,
                                1,
                                game.time * .7f);
                }
            },
            effects.ShadowPass());
        models.SetParentMatrix(IdentityMatrix());
    }

    void Overlays(const LevelOne& game)
    {
        effects.Unlit();
        glDepthMask(GL_FALSE);
        glLineWidth(2);
        int nearest = game.NearestDrop();
        ActorId nearestId = nearest >= 0 ? game.drops[nearest].Id() : 0;
        game.scene.Visit(
            [&](const Actor& actor, const ActorMatrix& world)
            {
                glPushMatrix();
                glMultMatrixf(world.data());
                if (const auto* dust = dynamic_cast<const DustActor*>(&actor))
                {
                    float t = dust->age / .8f;
                    Circle({}, .12f + t * .15f, .07f + t * .18f, .90f, .84f, .64f, (1 - t) * .5f);
                }
                else if (const auto* marker = dynamic_cast<const MarkerActor*>(&actor))
                {
                    if (marker->kind == MarkerKind::Spawn)
                    {
                        Circle({}, .09f, 5, .65f, .79f, .60f, .45f);
                    }
                    if (marker->kind == MarkerKind::Player)
                    {
                        Circle({}, .12f, .46f, .96f, .88f, .55f);
                    }
                    if (marker->kind == MarkerKind::Enemy)
                    {
                        const auto* enemy =
                            dynamic_cast<const LevelEnemy*>(game.scene.Find(actor.Parent()));
                        if (enemy && enemy->state == EnemyState::Windup)
                        {
                            Circle({}, .10f, 1.35f, 1, .39f, .27f);
                        }
                    }
                    if (marker->kind == MarkerKind::Attack && game.attackTime > 0)
                    {
                        glColor4f(1, .90f, .56f, game.attackTime / .36f);
                        glBegin(GL_TRIANGLE_STRIP);
                        for (int i = 0; i <= 24; ++i)
                        {
                            float angle = -1.4f + i * 2.8f / 24;
                            for (float radius : {1.55f, 2.25f})
                            {
                                glVertex3f(
                                    std::sin(angle) * radius, .65f, std::cos(angle) * radius);
                            }
                        }
                        glEnd();
                    }
                }
                else if (dynamic_cast<const LevelDrop*>(&actor))
                {
                    if (nearestId == actor.Id())
                    {
                        Circle({}, .1f, .5f, .92f, .82f, .52f);
                    }
                }
                glPopMatrix();
            });
        glLineWidth(1);
        glDepthMask(GL_TRUE);
    }
};

LevelOneView::LevelOneView() : implementation(new Implementation)
{
}

LevelOneView::~LevelOneView() = default;

void LevelOneView::Draw(const LevelOne& game, int width, int height)
{
    auto& view = *implementation;
    if (!game.FindPlayer())
    {
        glClearColor(.12f, .21f, .23f, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        view.ui.Draw(game, width, height, view.models.loadedFromDisk, view.models.savedToDisk);
        return;
    }
    view.effects.SetTime(game.time);

    if (view.effects.BeginShadow(game.camera.x, game.camera.z))
    {
        view.World(game);
    }

    view.effects.BeginScene(width, height);
    glClearColor(.76f, .83f, .79f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float aspect = float(width) / height;
    glOrtho(-12 * aspect, 12 * aspect, -12, 12, -100, 100);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glRotatef(35, 1, 0, 0);
    glRotatef(-45, 0, 1, 0);
    glTranslatef(-game.camera.x, 0, -game.camera.z);
    view.World(game);
    view.Overlays(game);
    view.effects.Composite();

    view.ui.Draw(game, width, height, view.models.loadedFromDisk, view.models.savedToDisk);
}
