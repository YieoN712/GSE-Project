#include "stdafx.h"
#include "Tutorial.h"
#include "RenderEffects.h"
#include "ModelCache.h"
#include "TutorialUi.h"
#include "Dependencies/glew.h"
#include "Dependencies/freeglut.h"
#include <algorithm>
#include <cmath>
#include <map>

namespace
{
    float Clamp(float v, float lo, float hi)
    {
        return (std::max)(lo, (std::min)(v, hi));
    }

    float Distance(GroundPoint a, GroundPoint b)
    {
        return std::hypot(a.x - b.x, a.z - b.z);
    }

    unsigned char Lower(unsigned char c)
    {
        return c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c;
    }
} // namespace

const GroundPoint Tutorial::board = {7, -11};
const GroundPoint Tutorial::bench = {8, 3};

Tutorial::Tutorial() : residents(scene), buildings(scene), footsteps(scene)
{
    Reset();
}

void Tutorial::Reset()
{
    if (!FindPlayer())
    {
        playerId = scene.Spawn<PlayerActor>(0).Id();
    }
    scene.Reparent(Player().Id(), 0);
    ActorList<MeshActor>(scene).clear();
    ActorList<MarkerActor>(scene).clear();
    Player().name = "Player";
    Player().active = Player().visible = true;
    Player().scale = {};
    Player().height = 0;
    scene.Reparent(Player().Id(), 0);
    Player().position = {-14, 9};
    camera = Player().position;

    stage = 0;
    elapsed = animation = stalled = 0;
    shadowTime = -1;
    paused = journal = completion = false;
    footsteps.clear();
    Player().heading = Player().stride = Player().speed = stepDistance = 0;

    dialogue.clear();
    line = 0;
    speaker = pendingStage = -1;
    ClearKeys();

    residents = {
        {L"빵집 사장님",
         L"오늘도 좋은 하루 보내. 빵 냄새가 골목 끝까지 나지?",
         {-12, 3},
         {-12, 3},
         .88f,
         .56f,
         .31f},
        {L"어르신", L"바람이 참 좋구나. 잠깐 쉬었다 가렴.", {8, 3}, {8, 3}, .56f, .49f, .72f},
        {L"청소하는 이웃",
         L"좋은 아침! 깨끗해진 길을 걸으면 기분도 좋아지지.",
         {-15, 8},
         {-15, 8},
         .28f,
         .63f,
         .65f},
        {L"꽃을 돌보는 주민",
         L"어제 심은 꽃이 폈어. 지나갈 때 한번 봐 줘.",
         {-3, -6},
         {-3, -6},
         .78f,
         .40f,
         .42f},
        {L"배달원",
         L"이 동네는 문 앞에 놓인 화분만 봐도 집을 찾을 수 있어.",
         {0, 7},
         {0, 7},
         .87f,
         .64f,
         .18f},
        {L"학생",
         L"게시판에 옛날 동네 사진이 있어. 우리 집도 나왔더라.",
         {5, -6},
         {5, -6},
         .31f,
         .47f,
         .73f},
        {L"편의점 손님",
         L"늘 먹던 음료가 있으면 하루가 조금 편해져.",
         {14, -3},
         {14, -3},
         .64f,
         .66f,
         .40f},
        {L"쉼터 이웃",
         L"여긴 오후 햇살이 가장 오래 머무는 곳이야.",
         {10, 5},
         {10, 5},
         .83f,
         .47f,
         .57f}};
    int residentSlot = 0;
    for (auto& resident : residents)
    {
        resident.slot = residentSlot++;
    }
    for (int i = 2; i <= 6; ++i)
        residents[i].heading = float(i) * 1.2f;
    residents[3].home = residents[3].position = {-24, -10};
    residents[6].home = residents[6].position = {28, 16};

    buildings = {{-17, -10, 8, 6, 4.4f, .91f, .80f, .65f, L"해온 주택"},
                 {-15, -2, 7, 6, 3.1f, .96f, .84f, .66f, L"온기 빵집"},
                 {-5, -13, 7, 5, 4.8f, .77f, .85f, .79f, L"세탁소"},
                 {16, -9, 7, 7, 3.5f, .82f, .87f, .82f, L"오늘 편의점"},
                 {17, 11, 6, 5, 3.0f, .94f, .77f, .66f, L"꽃집"},
                 {-7, 13, 7, 4, 2.8f, .87f, .83f, .70f, L"우리 집"}};

    // Outer blocks add explorable space while retaining the short tutorial route.
    for (int row = 0; row < 2; ++row)
        for (int col = 0; col < 5; ++col)
        {
            buildings.push_back({-34.f + col * 17.f,
                                 row == 0 ? -25.f : 25.f,
                                 8,
                                 6,
                                 3.5f + (col % 3),
                                 .79f + col * .025f,
                                 .80f - row * .06f,
                                 .73f,
                                 L"동네 상가"});
        }
    buildings.push_back({-35, -6, 7, 9, 4, .82f, .75f, .65f, L"주거동"});
    buildings.push_back({35, 8, 7, 9, 4, .85f, .80f, .69f, L"주거동"});
    BuildScenery();
    scene.FlushDestroyed();
}

void Tutorial::ClearKeys()
{
    keys.fill(false);
}

bool Tutorial::Blocked(GroundPoint p) const
{
    if (p.x < -HalfWidth || p.x > HalfWidth || p.z < -HalfDepth || p.z > HalfDepth)
        return true;
    for (const auto& b : buildings)
        if (std::abs(p.x - b.position.x) < b.width * .5f + .32f &&
            std::abs(p.z - b.position.z) < b.depth * .5f + .32f)
            return true;
    return false;
}

void Tutorial::Update(float dt)
{
    if (!FindPlayer())
    {
        return;
    }
    if (paused || completion)
        return;
    animation += dt;
    if (stage < 5)
    {
        elapsed += dt;
        stalled += dt;
    }
    // Cap simulation displacement without under-reporting real active play time.
    dt = (std::min)(dt, .05f);
    GroundPoint oldPlayer = Player().position;
    if (dialogue.empty() && scene.IsActive(Player().Id()))
    {
        float sx = float(keys['d']) - float(keys['a']);
        float sy = float(keys['w']) - float(keys['s']);
        float len = std::sqrt(sx * sx + sy * sy);
        if (len > 0)
        {
            // Inverse of the 45-degree camera yaw. Screen up means negative X and Z.
            float dx = (sx - sy) * .70710678f / len * 3.5f * dt;
            float dz = (-sx - sy) * .70710678f / len * 3.5f * dt;
            GroundPoint next = {Player().position.x + dx, Player().position.z};
            if (!Blocked(next))
                Player().position.x = next.x;
            next = {Player().position.x, Player().position.z + dz};
            if (!Blocked(next))
                Player().position.z = next.z;
        }
    }
    float moved = Distance(Player().position, oldPlayer);
    Player().speed = dt > 0 ? moved / dt : 0;
    if (moved > .0001f)
    {
        Player().heading =
            std::atan2(Player().position.x - oldPlayer.x, Player().position.z - oldPlayer.z);
        Player().stride += moved * 6;
        stepDistance += moved;
        if (stepDistance > .52f)
        {
            stepDistance = 0;
            footsteps.push_back({Player().position, 0});
        }
    }
    scene.Update(dt);
    if (!FindPlayer())
    {
        return;
    }
    footsteps.RemoveIf(
        [](const Footstep& footstep)
        {
            return footstep.age > .8f;
        });
    for (auto& n : residents)
    {
        if (n.slot < 2 || n.slot > 6)
        {
            continue;
        }
        if (!scene.IsActive(n.Id()))
        {
            continue;
        }
        GroundPoint before = n.position;
        float angle = n.heading;
        GroundPoint next = {n.position.x + std::sin(angle) * .65f * dt,
                            n.position.z + std::cos(angle) * .65f * dt};
        if (Blocked(next) || Distance(next, n.home) > 7.f)
        {
            angle += 1.4f;
            next = {n.position.x + std::sin(angle) * .65f * dt,
                    n.position.z + std::cos(angle) * .65f * dt};
        }
        if (!Blocked(next))
            n.position = next;
        n.heading = angle;
        n.walkSpeed = dt > 0 ? Distance(before, n.position) / dt : 0;
        n.stride += Distance(before, n.position) * 6;
    }
    camera.x += (Clamp(Player().position.x, -37, 37) - camera.x) * (1 - std::exp(-dt * 5));
    camera.z += (Clamp(Player().position.z, -26, 26) - camera.z) * (1 - std::exp(-dt * 5));
    if (stage >= 3 && shadowTime < 0 && Distance(Player().position, bench) < 5)
        shadowTime = 0;
    else if (shadowTime >= 0)
        shadowTime += dt;
    UpdateMarkers();
}

GroundPoint Tutorial::ObjectivePosition() const
{
    if (stage == 0)
    {
        const auto* resident = FindResident(0);
        return resident ? resident->position : bench;
    }
    if (stage == 1 || stage == 3)
        return bench;
    if (stage == 2)
        return board;
    const auto* resident = FindResident(7);
    return resident ? resident->position : bench;
}

const wchar_t* Tutorial::Objective() const
{
    static const wchar_t* text[] = {L"빵집 사장님에게 인사하기",
                                    L"쉼터의 어르신께 빵 전달하기",
                                    L"골목 게시판의 동네 사진 보기",
                                    L"쉼터의 빈 벤치 조사하기",
                                    L"쉼터 이웃에게 어르신에 대해 묻기",
                                    L"기억 기록: 오늘 빵을 건넨 사람"};
    return text[stage];
}

const wchar_t* Tutorial::TargetName(int target) const
{
    if (target >= 0 && target < 8)
    {
        const auto* resident = FindResident(target);
        return resident ? resident->name : L"";
    }
    return target == 8 ? L"동네 사진 게시판" : L"늘 앉던 자리";
}

int Tutorial::Target() const
{
    if (!FindPlayer() || !scene.IsActive(Player().Id()))
    {
        return -1;
    }
    int result = -1;
    float best = 1.8f;
    for (int i = 0; i < 10; ++i)
    {
        if (i >= 2 && i <= 6)
            continue; // Background walkers deliberately have no interaction.
        if (i == 1 && stage >= 3)
            continue;
        if (i == 9 && stage < 3)
            continue;
        const auto* resident = i < 8 ? FindResident(i) : nullptr;
        if (i < 8 && (!resident || !scene.IsActive(resident->Id())))
        {
            continue;
        }
        GroundPoint p = resident ? resident->position : (i == 8 ? board : bench);
        float d = Distance(Player().position, p);
        if (d >= best)
            continue;
        bool visible = true;
        for (int j = 1; j <= 8; ++j)
        {
            float t = j / 8.f;
            if (Blocked({Player().position.x + (p.x - Player().position.x) * t,
                         Player().position.z + (p.z - Player().position.z) * t}))
                visible = false;
        }
        if (visible)
        {
            best = d;
            result = i;
        }
    }
    return result;
}

void Tutorial::Say(int who, std::initializer_list<const wchar_t*> lines, int next)
{
    dialogue.assign(lines.begin(), lines.end());
    line = 0;
    speaker = who;
    pendingStage = next;
    ClearKeys();
}

void Tutorial::Interact()
{
    if (!dialogue.empty())
    {
        if (++line < dialogue.size())
            return;
        dialogue.clear();
        if (pendingStage >= 0)
        {
            stage = pendingStage;
            stalled = 0;
            if (stage == 5)
                completion = true;
        }
        pendingStage = -1;
        speaker = -1;
        return;
    }
    int t = Target();
    if (t < 0)
        return;
    if (t == 0 && stage == 0)
        Say(t,
            {L"벤치에 계신 어르신께 이 빵을 전해 줄래?",
             L"늘 같은 시간에 오시거든. 식기 전에 부탁해."},
            1);
    else if (t == 1 && stage == 1)
        Say(t,
            {L"오늘도 잊지 않았구나. 고맙다.",
             L"누군가 기다린다는 건 참 좋은 일이야.",
             L"돌아가는 길에 게시판의 동네 사진도 보고 가렴."},
            2);
    else if (t == 8 && stage == 2)
        Say(t,
            {L"사진 속 벤치에 어르신이 앉아 있다.",
             L"늘 지나가던 곳인데, 이렇게 보니 조금 다르다.",
             L"쉼터 쪽에서 바스락거리는 소리가 난다."},
            3);
    else if (t == 9 && stage == 3)
        Say(t,
            {L"빵 봉투와 손수건만 남아 있다.", L"사람은 없는데, 그림자는 조금 늦게 사라졌다."},
            4);
    else if (t == 7 && stage == 4)
        Say(t,
            {L"어르신? 오늘 저 벤치에는 아무도 없었는데.",
             L"분명 방금 이야기를 나눴는데….",
             L"잊지 않도록 오늘의 일을 기록해 두자."},
            5);
    else if (t == 8)
        Say(t, {L"동네 사람들이 함께 찍은 사진이다. 따뜻한 오후의 풍경."});
    else if (t == 9)
        Say(t, {L"오늘 빵을 건넨 사람. 그 목소리를 기억하고 싶다."});
    else
    {
        const auto* resident = FindResident(t);
        if (resident)
        {
            Say(t, {resident->greeting});
        }
    }
}

void Tutorial::KeyDown(unsigned char raw)
{
    unsigned char key = Lower(raw);
    if (keys[key])
        return;
    keys[key] = true;
    if (!FindPlayer())
    {
        if (key == 'r')
        {
            Reset();
        }
        return;
    }
    if (key == 27)
    {
        paused = !paused;
        ClearKeys();
        return;
    }
    if ((paused || completion) && key == 'r')
    {
        Reset();
        return;
    }
    if (completion && key == 'e')
    {
        completion = false;
        return;
    }
    if (paused || completion)
        return;
    if (key == 9)
        journal = !journal;
    if (key == 'e')
    {
        Interact();
        keys['e'] = true;
    }
}

void Tutorial::KeyUp(unsigned char key)
{
    keys[Lower(key)] = false;
}

void Tutorial::BuildScenery()
{
    auto mesh = [&](ModelKind model,
                    float x,
                    float y,
                    float z,
                    float sx,
                    float sy,
                    float sz,
                    float r,
                    float g,
                    float b,
                    int material) -> TutorialProp&
    {
        auto& actor = scene.Spawn<TutorialProp>(0);
        actor.name = "Neighborhood prop";
        actor.model = model;
        actor.position = {x, z};
        actor.height = y;
        actor.scale = {sx, sy, sz};
        actor.r = r;
        actor.g = g;
        actor.b = b;
        actor.material = material;
        return actor;
    };
    auto box = [&](float x,
                   float y,
                   float z,
                   float sx,
                   float sy,
                   float sz,
                   float r,
                   float g,
                   float b,
                   int material = 0) -> TutorialProp&
    {
        return mesh(ModelKind::Box, x, y, z, sx, sy, sz, r, g, b, material);
    };
    auto sphere = [&](float x,
                      float y,
                      float z,
                      float sx,
                      float sy,
                      float sz,
                      float r,
                      float g,
                      float b,
                      int material = 0) -> TutorialProp&
    {
        return mesh(ModelKind::Sphere, x, y, z, sx, sy, sz, r, g, b, material == 1 ? 9 : material);
    };
    box(0, -.4f, 0, 90, .35f, 66, .53f, .66f, .38f, 1);
    box(0, -.04f, 3, 88, .05f, 7, .40f, .44f, .45f, 2);
    box(5, -.03f, -2, 6, .05f, 62, .65f, .67f, .63f, 3);
    box(-12, -.02f, 7, 5, .05f, 46, .69f, .69f, .63f, 3);
    box(0, -.025f, -18, 86, .05f, 4, .43f, .46f, .47f, 2);
    box(0, -.025f, 19, 86, .05f, 4, .43f, .46f, .47f, 2);
    box(-25, -.025f, 0, 4, .05f, 63, .68f, .69f, .63f, 3);
    box(25, -.025f, 0, 4, .05f, 63, .68f, .69f, .63f, 3);
    for (int x = -21; x < 22; x += 2)
        box(float(x), .015f, 6.3f, 1.9f, .025f, .12f, .94f, .90f, .78f);
    for (int z = -15; z < 16; z += 2)
        box(2.2f, .016f, float(z), .10f, .024f, 1.9f, .94f, .90f, .78f);
    // Low props deliberately keep the central walking paths readable.
    for (int i = 0; i < 18; ++i)
    {
        float x = -41.f + i * 4.8f;
        float z = (i % 2) ? -30.f : 30.f;
        box(x, 0, z, .38f, 2.1f, .38f, .48f, .37f, .24f);
        float sway = std::sin(0.f + i) * .06f;
        sphere(x + sway, 2.6f, z, 1.5f, 1.1f, 1.4f, .41f, .62f, .37f, 1);
        sphere(x + sway, 3.25f, z, 1.05f, .85f, 1.0f, .56f, .72f, .42f, 1);
    }
    for (int i = 0; i < 7; ++i)
    {
        float x = -3.f + i * 2;
        box(x, 0, -8, .8f, .35f, .8f, .70f, .47f, .34f);
        box(x, .35f, -8, .75f, .27f, .75f, .41f, .61f, .35f);
        box(x, .62f, -8, .22f, .19f, .25f, .96f, .70f, .54f);
    }
    // A small sheltered garden and washing line give the neighborhood a lived-in scale.
    box(11, -.01f, 1, 5, .03f, 5, .60f, .72f, .43f, 1);
    box(12, 0, 0, .4f, 2.1f, .4f, .49f, .37f, .25f);
    sphere(12 + std::sin(0.f) * .05f, 2.9f, 0, 1.65f, 1.25f, 1.6f, .46f, .66f, .37f, 1);
    sphere(12, 3.8f, 0, 1.0f, .8f, .95f, .59f, .74f, .43f, 1);
    box(-19, 0, 11, .09f, 2.1f, .09f, .47f, .45f, .38f);
    box(-14, 0, 11, .09f, 2.1f, .09f, .47f, .45f, .38f);
    box(-16.5f, 1.99f, 11, 5, .025f, .025f, .45f, .43f, .37f);
    for (int i = 0; i < 4; ++i)
        box(-18.5f + i * 1.1f, 1.2f, 11, .8f, .8f, .025f, .93f, .85f - i * .05f, .72f, 12);
    for (int i = 0; i < 3; ++i)
    {
        float x = -19.f + i * 18;
        box(x, 0, 7, .15f, 2.7f, .15f, .31f, .39f, .38f);
        box(x, 2.7f, 7, .50f, .35f, .50f, .99f, .89f, .59f);
    }
    box(-12, .0f, 1.6f, 2, .65f, .8f, .58f, .39f, .26f);
    for (int i = 0; i < 4; ++i)
        box(-12.7f + i * .45f, .65f, 1.6f, .34f, .17f, .45f, .91f, .67f, .34f);
    // Bench, empty neighboring seat, and the handkerchief remain through the anomaly.
    box(8, .0f, 2.8f, 2.6f, .45f, .65f, .58f, .40f, .27f, 7);
    box(8, .45f, 2.5f, 2.6f, .50f, .12f, .66f, .47f, .30f, 7);
    box(8.8f, .46f, 2.9f, .36f, .02f, .34f, .92f, .91f, .78f);
    box(7.3f, .46f, 2.9f, .36f, .40f, .27f, .87f, .69f, .44f).minimumStage = 2;
    box(7, 0, -11, .13f, 2.2f, .13f, .50f, .38f, .26f);
    box(7, 1, -11, 2.5f, 1.3f, .15f, .55f, .42f, .29f);
    box(7, 1.15f, -10.90f, 2.1f, .96f, .03f, .94f, .90f, .75f);

    auto& carried = scene.Spawn<TutorialProp>(Player().Id());
    carried.name = "Bread bag";
    carried.position = {.48f, 0};
    carried.height = .5f;
    carried.scale = {.28f, .38f, .25f};
    carried.r = .88f;
    carried.g = .68f;
    carried.b = .40f;
    carried.minimumStage = carried.maximumStage = 1;
    scene.Spawn<MarkerActor>(Player().Id(), MarkerKind::Player);
    auto& anomaly = scene.Spawn<MarkerActor>(0, MarkerKind::Anomaly);
    anomaly.position = bench;
    scene.Spawn<MarkerActor>(0, MarkerKind::Objective);
    scene.Spawn<MarkerActor>(0, MarkerKind::Target);
    UpdateMarkers();
}

void Tutorial::UpdateMarkers()
{
    for (auto& marker : ActorList<MarkerActor>(scene))
    {
        if (marker.kind == MarkerKind::Objective)
        {
            marker.position = ObjectivePosition();
        }
        if (marker.kind == MarkerKind::Target)
        {
            int target = Target();
            marker.visible = target >= 0;
            if (target >= 0)
            {
                marker.position =
                    target < 8 ? FindResident(target)->position : (target == 8 ? board : bench);
            }
        }
    }
}

namespace
{
    RenderEffects* activeEffects = nullptr;
    ModelCache* activeModels = nullptr;
    ActorMatrix actorWorld = IdentityMatrix();

    void Color(float r, float g, float b, float a = 1)
    {
        glColor4f(r, g, b, a);
    }

    void Box(float x,
             float y,
             float z,
             float w,
             float h,
             float d,
             float r,
             float g,
             float b,
             int material = 0)
    {
        activeModels->Draw(ModelKind::Box, *activeEffects, x, y, z, w, h, d, 0, r, g, b, material);
    }

    void Disc(float x, float y, float z, float rx, float rz, float r, float g, float b, float a = 1)
    {
        if (activeEffects && activeEffects->ShadowPass())
            return;
        if (activeEffects)
            activeEffects->Material(0);
        activeEffects->ObjectTransform(actorWorld.data());
        glPushMatrix();
        glMultMatrixf(actorWorld.data());
        glNormal3f(0, 1, 0);
        Color(r, g, b, a);
        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(x, y, z);
        for (int i = 0; i <= 32; ++i)
        {
            float t = i * 6.2831853f / 32;
            glVertex3f(x + std::cos(t) * rx, y, z + std::sin(t) * rz);
        }
        glEnd();
        glPopMatrix();
    }

    void Ring(GroundPoint p, float radius, float r, float g, float b)
    {
        if (activeEffects && activeEffects->ShadowPass())
            return;
        if (activeEffects)
            activeEffects->Material(0);
        activeEffects->ObjectTransform(actorWorld.data());
        glPushMatrix();
        glMultMatrixf(actorWorld.data());
        glNormal3f(0, 1, 0);
        Color(r, g, b);
        glLineWidth(3);
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < 40; ++i)
        {
            float t = i * 6.2831853f / 40;
            glVertex3f(p.x + std::cos(t) * radius, .07f, p.z + std::sin(t) * radius);
        }
        glEnd();
        glLineWidth(1);
        glPopMatrix();
    }

    void Ellipsoid(float x,
                   float y,
                   float z,
                   float rx,
                   float ry,
                   float rz,
                   float r,
                   float g,
                   float b,
                   int material = 0)
    {
        activeModels->Draw(
            ModelKind::Sphere, *activeEffects, x, y, z, rx, ry, rz, 0, r, g, b, material);
    }

    void Person(GroundPoint p,
                float r,
                float g,
                float b,
                float time,
                int type,
                float heading = 0,
                float stride = 0,
                float speed = 0)
    {
        float swing = speed > .02f ? std::sin(stride) * .20f : 0;
        float bob = speed > .02f ? std::abs(std::sin(stride)) * .045f
                                 : std::sin(time * 1.8f + type) * .012f;
        // Transform each anatomical part into world space so it participates in the shadow map.
        auto part = [&](float x,
                        float y,
                        float z,
                        float rx,
                        float ry,
                        float rz,
                        float cr,
                        float cg,
                        float cb)
        {
            float wx = p.x + x * std::cos(heading) + z * std::sin(heading);
            float wz = p.z - x * std::sin(heading) + z * std::cos(heading);
            Ellipsoid(wx, y, wz, rx, ry, rz, cr, cg, cb, 8);
        };
        glDepthMask(GL_FALSE);
        for (int i = 4; i >= 0; --i)
            Disc(p.x,
                 .034f + i * .0004f,
                 p.z,
                 .29f + i * .035f,
                 .22f + i * .028f,
                 .20f,
                 .25f,
                 .28f,
                 .045f);
        glDepthMask(GL_TRUE);
        for (int side : {-1, 1})
        {
            float s = side * .14f, z = swing * side;
            part(s, .12f, z + .06f, .12f, .09f, .20f, .19f, .24f, .28f); // shoes
            part(s, .32f, z * .65f, .105f, .20f, .11f, .27f, .32f, .38f);
            part(s, .55f, z * .20f, .13f, .19f, .14f, .28f, .33f, .40f);
            part(
                side * .33f, .88f + bob, -z * .60f, .11f, .20f, .12f, r * .88f, g * .88f, b * .88f);
            part(side * .36f, .66f + bob, -z, .085f, .12f, .09f, .95f, .75f, .59f);
        }
        part(0, .89f + bob, 0, .29f, .31f, .19f, r, g, b);
        part(0, 1.18f + bob, 0, .09f, .10f, .09f, .95f, .75f, .59f);
        part(0, 1.40f + bob, 0, .23f, .25f, .215f, .96f, .77f, .61f);
        float hair = type == 1 ? .67f : .19f;
        part(0, 1.56f + bob, -.03f, .24f, .14f, .215f, hair, hair * .91f, hair * .83f);
        part(0, 1.40f + bob, .21f, .047f, .056f, .055f, .90f, .65f, .48f);
        for (int side : {-1, 1})
            part(side * .09f, 1.46f + bob, .193f, .023f, .025f, .023f, .15f, .19f, .20f);
        if (type == 0)
            part(0, .83f, .18f, .23f, .24f, .03f, .94f, .90f, .77f);
        if (type == 4 || type == 8)
            part(0, .92f, -.24f, .23f, .25f, .12f, .61f, .39f, .25f);
    }

} // namespace

TutorialView::TutorialView()
    : ui(new TutorialUi), models(new ModelCache), effects(new RenderEffects)
{
}

TutorialView::~TutorialView() = default;

void TutorialView::Draw(const Tutorial& game, int width, int height)
{
    ui->Prepare(width, height);
    if (!game.FindPlayer())
    {
        glClearColor(.12f, .21f, .23f, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        ui->Draw(game, width, height);
        return;
    }
    effects->SetTime(game.animation);
    activeEffects = effects.get();
    activeModels = models.get();
    auto world = [&]()
    {
        game.scene.Visit(
            [&](const Actor& actor, const ActorMatrix& transform)
            {
                actorWorld = transform;
                models->SetParentMatrix(transform);
                if (const auto* prop = dynamic_cast<const TutorialProp*>(&actor))
                {
                    if (game.stage < prop->minimumStage || game.stage > prop->maximumStage)
                    {
                        return;
                    }
                    models->Draw(prop->model,
                                 *effects,
                                 0,
                                 0,
                                 0,
                                 1,
                                 1,
                                 1,
                                 0,
                                 prop->r,
                                 prop->g,
                                 prop->b,
                                 prop->material);
                }
                else if (const auto* building = dynamic_cast<const Building*>(&actor))
                {
                    Building b = *building;
                    b.position = {};
                    float forward = (transform[12] - game.Player().position.x + transform[14] -
                                     game.Player().position.z) *
                                    .7071f;
                    float sideways = std::abs((transform[12] - game.Player().position.x -
                                               transform[14] + game.Player().position.z) *
                                              .7071f);
                    if (forward > 0 &&
                        forward < b.buildingHeight * 1.8f + (b.width + b.depth) * .36f &&
                        sideways < (b.width + b.depth) * .36f)
                    {
                        Box(0, 0, 0, b.width, .20f, b.depth, b.r, b.g, b.b);
                        return;
                    }
                    Box(b.position.x,
                        0,
                        b.position.z,
                        b.width,
                        b.buildingHeight,
                        b.depth,
                        b.r,
                        b.g,
                        b.b,
                        4);
                    Box(b.position.x,
                        b.buildingHeight,
                        b.position.z,
                        b.width + .4f,
                        .25f,
                        b.depth + .4f,
                        .46f,
                        .49f,
                        .47f,
                        5);
                    Box(b.position.x + 1,
                        b.buildingHeight + .25f,
                        b.position.z,
                        1.1f,
                        .65f,
                        .8f,
                        .76f,
                        .79f,
                        .76f,
                        5); // roof equipment
                    for (float x = b.position.x - b.width / 2 + 1; x < b.position.x + b.width / 2;
                         x += 1.7f)
                    {
                        Box(x,
                            1.4f,
                            b.position.z + b.depth / 2 + .035f,
                            1.05f,
                            1.1f,
                            .08f,
                            .33f,
                            .54f,
                            .55f,
                            6);
                        Box(x,
                            1.4f,
                            b.position.z + b.depth / 2 + .09f,
                            .055f,
                            1.1f,
                            .04f,
                            .80f,
                            .82f,
                            .79f,
                            5);
                        Box(x,
                            1.37f,
                            b.position.z + b.depth / 2 + .10f,
                            1.17f,
                            .08f,
                            .15f,
                            .94f,
                            .90f,
                            .75f);
                    }
                    Box(b.position.x,
                        0,
                        b.position.z + b.depth / 2 + .07f,
                        .85f,
                        1.8f,
                        .1f,
                        .38f,
                        .36f,
                        .30f);
                    const std::wstring sign = b.sign;
                    if (sign == L"온기 빵집" || sign == L"오늘 편의점" || sign == L"꽃집")
                    {
                        Box(b.position.x,
                            2.25f,
                            b.position.z + b.depth / 2 + .4f,
                            b.width - .5f,
                            .15f,
                            1.2f,
                            .30f,
                            .59f,
                            .57f);
                        for (int i = 0; i < 5; ++i)
                        {
                            Box(b.position.x - 2 + i,
                                2.23f,
                                b.position.z + b.depth / 2 + .43f,
                                .30f,
                                .18f,
                                1.22f,
                                .91f,
                                .85f,
                                .65f);
                        }
                    }
                }
                else if (const auto* resident = dynamic_cast<const Resident*>(&actor))
                {
                    int index = resident->slot;
                    if (index == 1 && game.stage >= 3)
                    {
                        return;
                    }
                    Person({},
                           resident->r,
                           resident->g,
                           resident->b,
                           game.animation,
                           index,
                           0,
                           resident->stride,
                           resident->walkSpeed);
                }
                else if (dynamic_cast<const PlayerActor*>(&actor))
                {
                    Person({},
                           .27f,
                           .64f,
                           .72f,
                           game.animation,
                           8,
                           0,
                           game.Player().stride,
                           game.Player().speed);
                }
                else if (const auto* footstep = dynamic_cast<const DustActor*>(&actor))
                {
                    glDepthMask(GL_FALSE);
                    float t = footstep->age / .8f;
                    for (int i = 0; i < 3; ++i)
                    {
                        Disc((i - 1) * t * .16f,
                             .08f + t * .15f,
                             t * .13f,
                             .05f + t * .12f,
                             .04f + t * .10f,
                             .85f,
                             .81f,
                             .67f,
                             (1 - t) * .24f);
                    }
                    glDepthMask(GL_TRUE);
                }
                else if (const auto* marker = dynamic_cast<const MarkerActor*>(&actor))
                {
                    if (marker->kind == MarkerKind::Player)
                    {
                        Ring({}, .49f, .96f, .92f, .68f);
                    }
                    if (marker->kind == MarkerKind::Target)
                    {
                        Ring({}, .57f, .95f, .98f, .84f);
                    }
                    if (marker->kind == MarkerKind::Anomaly && game.stage >= 3)
                    {
                        Disc(0, .045f, 0, 2, 1.6f, .42f, .56f, .61f, .28f);
                        if (game.shadowTime >= 0 && game.shadowTime < 2)
                        {
                            Disc(
                                0, .051f, 0, .32f, 1.1f, .20f, .24f, .30f, 1 - game.shadowTime / 2);
                        }
                    }
                    if (marker->kind == MarkerKind::Objective && game.stage < 5)
                    {
                        float pulse = std::sin(game.animation * (game.stalled > 30 ? 5 : 2)) * .12f;
                        Ring({}, .80f + pulse, .96f, .76f, .31f);
                        Box(0, 2.2f + pulse, 0, .20f, .50f, .20f, .98f, .80f, .35f);
                    }
                }
            },
            effects->ShadowPass());
        models->SetParentMatrix(IdentityMatrix());
        effects->ObjectTransform(nullptr);
    };
    if (effects->BeginShadow(game.camera.x, game.camera.z))
    {
        world();
    }
    effects->BeginScene(width, height);
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
    world();
    effects->Unlit();
    activeEffects = nullptr;
    activeModels = nullptr;
    ui->DrawWorldLabels();
    effects->Composite();
    ui->Draw(game, width, height);
}

PlayerActor* Tutorial::FindPlayer()
{
    return dynamic_cast<PlayerActor*>(scene.Find(playerId));
}

const PlayerActor* Tutorial::FindPlayer() const
{
    return dynamic_cast<const PlayerActor*>(scene.Find(playerId));
}

PlayerActor& Tutorial::Player()
{
    auto* actor = FindPlayer();
    if (!actor)
    {
        throw std::logic_error("Tutorial player has been removed.");
    }
    return *actor;
}

const PlayerActor& Tutorial::Player() const
{
    const auto* actor = FindPlayer();
    if (!actor)
    {
        throw std::logic_error("Tutorial player has been removed.");
    }
    return *actor;
}

const Resident* Tutorial::FindResident(int slot) const
{
    for (const auto& resident : residents)
    {
        if (resident.slot == slot)
        {
            return &resident;
        }
    }
    return nullptr;
}
